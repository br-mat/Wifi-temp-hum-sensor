# Temperature and humidity sensor rework design

Status: draft for owner review, 2026-11-11. No implementation is approved by this document. Numeric defaults below are proposals until reviewed or measured.

## Purpose and scope

Replace the ESP-01 and DHT11 with a small battery powered node that retains direct Wi-Fi writes to InfluxDB and offers BLE telemetry to the existing Raspberry Pi as an alternative. Use this build to establish a simple BLE receiving path reusable in other projects. The first deployment is indoor environmental monitoring.

Wi-Fi goes directly from the node to InfluxDB. Node-RED is exclusively an option for the BLE receiving path. Select one telemetry mode per node; simultaneous transmission, automatic failover, phone configuration, OTA updates, and recovery of missed historical samples are outside the first version.

## Proposed hardware

| Function | Proposal | Design consequence |
| --- | --- | --- |
| Controller | Seeed XIAO ESP32C3 | Wi-Fi and BLE on one USB programmable board. Confirm exact revision before final wiring and CAD. |
| Temperature and humidity | Sensirion SHT41 on a documented breakout | I2C at 3.3 V; no unnecessary LEDs or permanently powered accessories. Place away from heat sources. |
| Supply | Protected single-cell 3.7 V nominal Li-ion/LiPo, charged to its specified limit | Use the board's battery input and compatible charging circuit. Exact cell and capacity remain open. |
| Battery measurement | External divider into a suitable ADC1 pin, with filtering | Validate input range at maximum cell voltage, resistor tolerances, ADC settling, calibration, and sleep leakage. |
| Mechanics | Serviceable enclosure with ventilation, USB access, and power switch | Freeze dimensions after selecting the cell, antenna position, and thermal layout. |

The ESP32C3 board has Wi-Fi and BLE and a battery charger. Seeed lists approximately 380 mA fast charging; the selected cell must explicitly permit the actual board's charge current. Its documentation also describes external battery-voltage sensing because no onboard battery ADC connection is provided. Use ADC1 for reliable analog readings. Confirm the exact revision's charger, protection, regulation, and power-path behavior before wiring. [Seeed hardware documentation](https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/)

The SHT41 supports I2C and a supply compatible with 3.3 V operation. A breakout may add its own regulator or other loads. Choose the actual board from its schematic, rather than assuming chip sleep consumption describes the complete module. [Sensirion SHT41](https://sensirion.com/products/catalog/SHT41)

Start with a simple high-value divider and filter capacitor if its measured leakage and settling are acceptable. Consider switching the divider only if needed; switching must not introduce a leakage or back-power path. Pin assignments and resistor/capacitor values belong in the reviewed electrical design. Battery voltage must never be connected directly to a 3.3 V GPIO.

## Measurement scheme

1. Wake on timer or manual recovery/reset. Restore the selected configuration and initialize the required peripherals with timeouts.
2. With radios off, read battery voltage first. Allow the divider/filter to settle, average a small set of ADC conversions, apply ADC calibration and the measured divider ratio, and check plausibility. This averaging treats ADC noise; it is separate from environmental sampling. A confirmed critical battery bypasses sensing and radio work for a longer battery-recheck sleep.
3. Trigger one fresh SHT41 conversion at the driver's high-precision setting. Wait for completion according to the sensor specification; read temperature and humidity together and validate the response/CRC.
4. If the sensor operation fails, allow one fresh retry within a bounded sensor budget. If it still fails, omit temperature/humidity from the report. Never substitute zero or label cached data as fresh.
5. Build a fresh sample containing valid measurements and internal health information. Send it through the selected route, then stop radio activity and enter timer deep sleep.

Keep the sensor heater disabled during normal operation. Heater/decontamination use would be a separately specified maintenance operation, with affected environmental readings excluded. Start without median filtering or smoothing; retain raw valid readings and do display aggregation in Grafana. Calibration offsets require comparison with a suitable reference, not adjustment to make two unverified sensors agree.

Measure before enabling the radio to reduce immediate self-heating effects. Verify thermal bias experimentally after enclosure assembly and during charging. A single valid conversion does not establish absolute system accuracy.

## ESP32 execution and deep sleep

Use a single application state sequence for one wake cycle, then timer deep sleep. Start with Arduino C++ and small functions for configuration, battery sampling, environmental sampling, each radio transport, and sleep preparation. Do not add application tasks or a scheduler framework merely to perform this sequence; radio libraries may use their own internal tasks.

With Arduino, `setup()` starts the cycle after both cold boot and timer wake. Deep sleep restarts normal program execution on wake rather than resuming after the sleep call. Keep `loop()` as a defensive fallback that enters a safe timed sleep if the main cycle unexpectedly returns. Ordinary heap objects and driver state must be rebuilt each wake. [Espressif sleep documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-reference/system/sleep_modes.html)

```text
Boot or timer wake
  -> classify wake/reset; establish cycle deadline; load and validate configuration
  -> check explicit development/recovery mode
  -> read battery with radios off
       critical -> skip sensing/radio -> long battery-recheck sleep
       invalid  -> mark battery unavailable; use conservative bounded operation
       normal   -> continue
  -> acquire fresh SHT41 sample, with at most one bounded retry
  -> freeze valid sample and advance report counter
  -> selected transport
       Wi-Fi -> connect -> direct InfluxDB write -> stop Wi-Fi
       BLE   -> broadcast -> bounded connectable window
                -> optional authorized interval write and confirmed readback
                -> stop BLE
  -> release resources; set safe GPIO states; retain small diagnostic state
  -> compute remaining time to next scheduled wake
  -> arm RTC timer -> enter deep sleep
```

An invalid ADC reading is not proof of a critically low battery. Omit it, record the failure, and keep the awake-time limit; exact conservative behavior and repeated-brownout backoff need verification with the electrical design. If the sensor fails but battery operation is permitted, still send available battery/health data. A Wi-Fi failure ends in sleep, never an immediate return to an unbounded reconnect loop.

### State retained between wakes

| State | Storage | Rules |
| --- | --- | --- |
| Accepted interval override, configuration version, and other durable settings | NVS; Arduino Preferences for small settings | Load and validate each wake. Write only a real accepted change. Acknowledgement of persistence requires a successful storage operation. |
| BLE bonding data, if enabled | BLE stack's persistent storage | Follow the selected stack's lifecycle; do not erase on every wake. Commissioning/recovery still needs design. |
| Report counter and small diagnostic counters | RTC retained memory | Version/validate the retained structure. Reset safely after cold boot, invalid retention, or relevant reset causes. Loss is acceptable for diagnostics. |
| Current readings, drivers, network clients, buffers | Normal RAM | Recreate on every wake; do not retain pointers or connection objects. |

Preferences uses ESP32 NVS and retains values across restarts and power loss. RTC retention only serves continuity across supported sleep/reset paths, not durable configuration. Do not write flash every 30 minutes just to maintain a sample counter. [Preferences documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/preferences.html)

The GATT callback validates/stages a bounded request for the application sequence to process. Serialize settings updates; do not let a callback race sleep preparation or confirm a value before durable storage succeeds. A readback must distinguish a pending request from the persisted effective value. A timed-out connection does not undo a change that was already saved; the Pi can confirm it at the next wake.

### Deadlines and shutdown

The normal cycle has one proposed 15-second awake deadline, including cleanup. Give each operation a timeout no longer than the remaining budget, reserving time to stop the active radio and arm sleep. Wi-Fi association's ten-second cap, BLE's five-second discovery window, and a connection transaction's ten-second cap are maxima, not time allocations that can be added together. After two seconds of BLE telemetry and a five-second wait, a late connection receives only the remaining transaction budget. If that is insufficient in practice, review the total budget from measurements rather than silently exceeding it.

Keep libraries' blocking calls, I2C, time synchronization, DNS, HTTP/TLS, and BLE connection work within explicit bounds. Keep the platform watchdog active as a recovery mechanism, but do not treat watchdog resets as the normal scheduler. Record repeated failure/reset behavior and validate backoff so a fault cannot create a high-current reboot loop.

Before sleeping, stop/disconnect only the transport that was initialized, release sensor/ADC resources as needed, and leave external circuitry in its measured low-leakage state. Keep the SHT41 supplied in its idle state initially; add power switching only if actual savings justify it. A powered-down sensor can be back-powered through I2C, so any later switching design must address pull-ups and GPIO states.

The effective interval is 1800 seconds unless overridden. Use 64-bit timing arithmetic: compute elapsed awake time, choose the next future multiple of the effective interval from this wake, then arm the timer for the remaining duration in microseconds. Normal cycles sleep for approximately interval minus awake time. An overrun skips elapsed slots, rather than creating a burst of catch-up reports. The cycle timer and heap do not provide continuity across deep sleep; this schedule is relative and subject to RTC drift, not aligned wall-clock sampling.

An accepted BLE interval change is used in that sleep calculation immediately. A critical-battery recheck interval is a separate policy, to be selected with battery thresholds. In that state remote changes are unavailable because the radio is deliberately off.

Use an explicit development build option or deliberate maintenance input to keep the node awake for flashing/log inspection. Production must not wait indefinitely for a USB serial host, keep an LED on, or skip sleep just because USB is attached. Validate wake, radio reinitialization, retained state, and actual sleep current over repeated cycles with USB disconnected.

## Schedule and time

| Setting | Draft default | Reason |
| --- | --- | --- |
| Deployment sample/report interval | 30 minutes for either route | Owner-selected default; a valid explicit setting may override it. |
| Bench sample interval | 10 seconds | Speeds debugging; mark as a development setting. |
| Battery report interval | Every environmental report | Small extra payload; gives voltage history with consistent sampling conditions. |
| BLE advertising window | 2 seconds, approximately 250 ms advertising interval | Repeat the same sample several times; tune against measured reception and charge use. |
| Wi-Fi association timeout | At most 10 seconds | Prevent indefinite awake time when the access point is absent. |
| Total awake budget | Initial target 15 seconds | Includes sensing, connection, time setup if required, and upload. Every blocking operation needs its own bounded timeout. |
| Stale-device indication | 3 missed nominal report intervals | At the default, flag no recent report after roughly 90 minutes plus a small receiver allowance. Use the confirmed effective interval when overridden. |

Take and report one sample each cycle. Do not batch samples in version one. Measure cycle duration and sleep for the remaining interval so the schedule approximates wake-to-wake cadence rather than adding a full interval after transmission. If a cycle overruns, skip missed slots rather than creating a catch-up loop. Wake immediately for the next normal cycle after a cold start. Account for a device's configured interval when assessing staleness.

Wi-Fi connects once per report and sends one report; no tight reconnect loop or indefinite retry. If delivery fails, retain local diagnostic counters where practical and try again at the next scheduled cycle. A request whose response is lost can still have reached InfluxDB: delivery is not exactly once, and version one must not claim it is. Historical buffering and idempotent retries are later work.

Use InfluxDB server receive time for direct Wi-Fi writes in the first version unless the selected client requires explicit timestamps. Use first gateway receive time for a BLE sample, and preserve that timestamp during any later gateway retry. These are arrival times, not precision acquisition timestamps. Both Pi clock and time policy must be checked. HTTPS additionally needs the chosen library's trusted-time/certificate requirements satisfied; do not disable certificate verification to avoid that work. Arrival-time storage alone does not remove TLS time requirements.

## Configurable interval and optional BLE control

The default is 1800 seconds. Proposed setting semantics:

- An absent setting uses the default. A valid saved override survives normal deep sleep, reset, power loss, and a Pi/Node-RED outage.
- A proposed production range is 60 to 86400 whole seconds, pending review. Development-only fast sampling is separate.
- Invalid input is rejected without changing the active setting. Zero is not an interval; a distinct reset-to-default action removes the override.
- Save only an accepted change, not every wake. Return/read back the accepted value and use it to schedule the next cycle after the current report completes.
- Distinguish the requested interval from the device-confirmed effective interval. A Node-RED edit alone must not be displayed as a successful device change. Read back after an uncertain write, and let repeating the same requested value be harmless.
- Basic Wi-Fi mode uses local configuration; it continues writing directly to InfluxDB. Remote BLE control applies only to BLE mode unless a later design explicitly expands that scope.

The owner selected remote BLE interval changes at the next scheduled wake, accepting the extra receive time. BTHome advertisements carry telemetry; they do not themselves provide a command return path. Add a small GATT configuration service so the Pi can connect and write/read the interval during a deliberately bounded window at each scheduled BLE wake. The ESP32C3 cannot maintain its BLE connection in deep sleep. [Espressif sleep documentation](https://docs.espressif.com/projects/esp-idf/en/release-v5.2/esp32c3/api-reference/system/sleep_modes.html)

| Configuration opportunity | Behavior | Tradeoff |
| --- | --- | --- |
| Physical button or deliberate maintenance wake | Open a short connectable window, provisionally 60 seconds; Node-RED/Pi applies the setting then | No scheduled receive window, but someone must reach the device. Reserve a suitable pin and enclosure access if chosen. |
| Every scheduled BLE wake | Pi retains a pending setting and connects when the device next advertises a configuration service | Unattended changes, normally delayed up to the current interval; extra energy and a longer, measured wake window. An offline node cannot be reached. |
| USB only initially | Set the interval locally and retain 30 minutes when unset | Simplest first version; Node-RED configuration is deferred. |

The scheduled-wake option is selected; the other rows document alternatives considered. Start with a proposed five-second connectable window per scheduled BLE wake, separate from the two-second telemetry broadcast. Keep it open even when no change is pending, because the node cannot know whether the Pi has a request. Tune its duration from actual Pi discovery/connection latency and battery measurements. A proposed ten-second total connection-transaction limit must fit within the normal awake budget; verify this with the chosen stack. On timeout, disconnect, preserve the last valid setting, and sleep rather than extending the window indefinitely.

At each normal BLE wake: take measurements, broadcast the sample, advertise the configuration service, accept an authorized Pi connection if present, validate and persist an actual setting change, return the effective interval, disconnect, and schedule the next wake. A successful change controls the interval from this cycle's wake to the next; subtract elapsed awake time as in the normal scheduler. A rejected or absent change leaves the current setting intact. If confirmation is lost, the Pi reads back on a later wake before treating the request as complete. No pending request requires no Pi connection. If the Pi is unavailable, the node continues on its current interval. Critical-battery protection can skip the normal BLE cycle entirely.

Do not implement a permanent connection or assume a sleeping device can receive a command. Authorize configuration writes with authenticated pairing/bonding to the intended Pi; the exact commissioning and recovery procedure remains to be designed. A nearby device must not be able to change settings silently. Define disconnect, timeout, pairing/recovery, and write-confirmation behavior before adding the service.

If Node-RED is used, extend the BLE flow only with a simple interval input, validation, pending request, write, and confirmed readback. Keep one pending target value per node; a later owner edit replaces it. Avoid importing a dashboard package or general command framework. The BLE connection mechanism still depends on the installed palette or selected small helper. This control extension does not alter the direct Wi-Fi route.

Thirty-minute sampling provides periodic snapshots and can miss short temperature/humidity events. If that becomes a problem, choose a shorter explicit interval; separate fast sampling and infrequent batch uploads would require another reviewed design.

## Battery management

The first firmware reports voltage, not an invented state-of-charge percentage. Battery voltage depends on load, temperature, cell chemistry, and charging, so it is useful for trends and warnings but insufficient for a precise percentage. Add a fuel gauge only if a reliable percentage or remaining-runtime estimate becomes a requirement.

| State | Proposed behavior |
| --- | --- |
| Normal | Use the configured schedule and report valid measurements and battery voltage. |
| Low | Report a warning where supported. A later adaptive policy may lengthen the effective interval, for example doubling it, but must never shorten it relative to the configured interval. Use hysteresis so voltage noise does not toggle the mode. |
| Critical | Stop regular radio operation and use long sleeps with occasional battery rechecks. Do not repeatedly attempt Wi-Fi at a voltage that causes brownouts. |
| USB powered or charging | Keep normal reporting if desired; identify charging only when hardware provides a verified signal. Treat measurements taken during charging as potentially thermally biased. |

Low, critical, and recovery thresholds remain deliberately open until the chosen cell, regulator dropout, measured radio voltage sag, and ADC error are known. Firmware thresholds are operational safeguards; the cell's protection circuit provides independent hardware protection. Long sleeps still draw current, so manual disconnection remains appropriate for storage. Charging behavior and permissible temperatures follow the selected cell and charger specifications.

Use a protected cell and verify whether the actual board has suitable protection and load sharing. Check charging with the node running; do not assume a charger guarantees ideal power-path management. The owner has left the format open: compare pouch and 18650 options by usable capacity, measured runtime, physical dimensions, holder/connector size, charging compatibility, and serviceability before choosing. Do not place the sensor beside a warm charger or cell.

Measure battery-side charge over the complete cycle with USB disconnected in each telemetry mode. Include sleep leakage, sensing, startup, association, time synchronization, encryption/TLS if used, and failures. Size capacity from usable mAh divided by average mA, with explicit derating. Six months remains an aspiration from the private plan, not an established requirement or runtime claim. Wi-Fi and BLE may need different intervals/capacities to reach it.

## Telemetry contents

| Item | Wi-Fi direct write | Optional BLE | Storage/display rule |
| --- | --- | --- | --- |
| Temperature | Valid degrees Celsius | BTHome temperature object | Omit on failure; never use zero as an error value. |
| Relative humidity | Valid percent RH | BTHome humidity object | Omit on failure. |
| Battery voltage | Volts | BTHome voltage object | Send only if measurement is valid and calibrated. |
| Device identity | Stable configured device ID | Stable BLE identity mapped by the Pi | Same logical ID and location in both routes. |
| Sample counter | Diagnostic field if useful | BTHome packet ID | Change per fresh report, repeat unchanged within its BLE advertising window. |
| Signal strength | Wi-Fi RSSI, if useful | RSSI measured by Pi receiver | Distinct field names; do not compare them as one signal. |
| Firmware version and reset cause | Boot/service diagnostics | Local serial diagnostics initially | Do not invent custom BLE fields to force parity. |
| Sensor/transport failure counts | Health fields when delivery is possible | Serial logs and missing environmental fields initially | A battery-only report can indicate a live node with missing sensor data. |
| Low battery | Local state and health field | Dedicated standard object if selected during payload review | No battery percentage without a defined estimation method. |

Use standard BTHome v2 objects for temperature, humidity, voltage, and packet ID. The format supplies these objects and supports voltage in volts with millivolt encoding resolution; encoding resolution is not ADC accuracy. Keep the chosen objects within the advertising size budget and verify the exact byte layout before implementation. [BTHome specification](https://bthome.io/format/)

Start with InfluxDB measurement `environment`, tags `device_id` and `location`, fields `temperature`, `humidity`, and `battery_voltage`. Use explicit units in the documentation and Grafana. Optional fields such as `wifi_rssi` and `ble_rssi` belong to their respective routes. Diagnostic counters are fields, not tags. BLE should not need to reproduce every Wi-Fi diagnostic field to share the useful environmental schema.

Track last report received and last valid environmental measurement separately. A battery-only report must not make a failed temperature sensor appear healthy. Low-battery slowdown must remain visible to the receiver so it can apply the right stale threshold; define that indication before enabling adaptive schedules. Until then, use the normal cadence and warnings only.

## Telemetry routes

```text
SHT41 + battery ADC -> validate sample -> configured transport
                                          |
                                          +-- Wi-Fi -> InfluxDB -> Grafana
                                          |
                                          +-- BLE -> Pi receiver -> InfluxDB
                                                      |
                                             Node-RED is an option
```

Wi-Fi requires local network credentials and a narrowly scoped database token kept outside tracked files. Match the client's API and authentication to the actual installed InfluxDB version. Decide the LAN HTTP/HTTPS policy explicitly during integration; use trusted certificate validation if HTTPS is selected.

For BLE, the Pi continuously scans while the node only broadcasts for the bounded window. Deduplicate by stable node identity and a short-lived packet-ID cache; handle counter wrap, restarts, and repeated advertisements. A broadcast is not acknowledged. Samples missed while the Pi is offline remain gaps.

Inspect the Pi's installed Node-RED nodes before choosing BLE input. If practical, use a small flow: input, decode/validate, identity mapping/deduplication, formatting, HTTP Request to InfluxDB, plus visible errors. Avoid imported flow bundles, fancy modules, dashboards, and unnecessary contributed nodes. A small standalone receiver is acceptable if simpler. Assign decoding/deduplication to one component. Basic reception comes first; durable queueing is a separate reviewed reliability increment.

BLE bench packets may be unencrypted. Decide privacy and spoofing requirements before deployment; if encryption is chosen, specify key handling and counter persistence across reboot and power loss. No automatic Wi-Fi/BLE failover or dual-route deduplication is required initially.

## Other design decisions to settle

- Exact controller revision, sensor breakout, cell format/capacity, charge current, switch placement, ADC circuit, and antenna clearance.
- Runtime goal and acceptable reporting interval in each route, with actual intended radio range through walls.
- USB recovery path and a deliberate development mode that allows serial inspection before entering sleep.
- Configuration format for identity, location, interval, transport, and credentials; begin with a simple local build configuration and a tracked example.
- Optional BLE interval control: scheduled receive window selected. Finalize window/transaction budgets, commissioning and write authorization, persistence, confirmation, and interval reporting before implementing it.
- Sensor error indication, low-battery thresholds/hysteresis, brownout recovery, and persistent state without unnecessary flash writes.
- Trusted-time and HTTP/HTTPS policy, current InfluxDB version, bucket/retention, and write-only token scope.
- Receiver choice and its restart behavior, timestamp/duplicate policy, and outage behavior.
- Enclosure ventilation, sensor thermal isolation, battery serviceability, USB access, and mounting. Outdoor use requires another design pass.

## Review and implementation order

1. Review this design and settle hardware candidates, cadence, battery format, and minimum data fields.
2. Inventory the actual Pi services and selected hardware; document wiring and build/flash commands.
3. Prove fresh sensor and calibrated battery measurements over USB.
4. Prove direct Wi-Fi writes with bounded failure behavior.
5. Add optional BLE telemetry and the simplest Pi receiver.
6. Measure both power profiles, then finalize battery thresholds, capacity, and mechanics.
7. Verify enclosure bias, intended range, missing-data behavior, and receiver restart; draft the next small reviewed increment.

Work on `dev`. Present each proposed commit's scope, diff, verification results, and message to the owner. Commit only after explicit approval. This phase adds documentation only.
