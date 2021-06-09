#ifndef PAGE_ROOT_DASHBOARD_
#define PAGE_ROOT_DASHBOARD_

const char PAGE_ROOT[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta http-equiv="X-UA-Compatible" content="IE=edge">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">

  <title>Smart Parking Dashboard</title>

  <link rel="preconnect" href="https://fonts.gstatic.com">
  <link href="https://fonts.googleapis.com/css2?family=Nunito:ital,wght@0,300;0,400;0,600;0,700;1,300;1,400;1,600;1,700&display=swap" rel="stylesheet">
  <link href="https://unpkg.com/tailwindcss@^2/dist/tailwind.min.css" rel="stylesheet">
  <link rel="stylesheet" href="https://unicons.iconscout.com/release/v3.0.6/css/line.css">
  <style>
  body { background: #f0f1f4; }
  h1, h2, h3, h4, h5, h6, .nunito { font-family: 'Nunito'; }
  th { text-align: left }
  </style>
</head>
<body style="min-width: 320px;">
  <div id="app">
    <div class="container-fluid bg-white py-6 mb-8 border-b border-solid shadow-sm">
      <div class="container mx-auto max-w-screen-lg px-8 px-4">
        <h3 class="text-3xl font-bold">Smart Parking Dashboard</h3>
        <h5 class="text-gray-500 font-semibold">Laboratorio IoT@Unimib</h5>
      </div>
    </div>

    <div class="container mx-auto my-2 max-w-screen-lg px-8 my-4">
      <h2 class="ml-3 mb-1 text-2xl font-semibold flex items-center">
        Controls
        <span v-if="loading" class="uppercase text-gray-600 text-xs mx-1 py-1 inline-block">(updating)</span>
      </h2>
      <div class="flex gap-2">
        <div class="flex flex-row bg-white shadow-sm items-center rounded p-3 h-full cursor-pointer" :class="{'cursor-not-allowed': !('display' in status)}" @click="displayToggle">
          <div class="flex items-center justify-center flex-shrink-0 h-12 w-12 rounded bg-gray-100 text-gray-300 transition"
            :class="{'bg-green-100': status.display, 'text-green-500': status.display}">
            <i class="uil uil-desktop text-2xl"></i>
          </div>
          <div class="flex flex-col flex-grow ml-2 justify-center">
            <div class="text-sm text-gray-600 font-bold nunito">External Display</div>
            <div class="font-semibold uppercase text-gray-400 transition" :class="{ 'text-green-500': status.display }">
              <span v-if="!('display' in status)">OFFLINE</span>
              <span v-else-if="status.display">On</span>
              <span v-else>Off</span>
            </div>
          </div>
        </div>

        <div class="flex flex-row items-center bg-white shadow-sm rounded p-3 h-full cursor-pointer" :class="{'cursor-not-allowed': !('lights' in status)}" @click="lightsToggle">
          <div class="flex items-center justify-center flex-shrink-0 h-12 w-12 rounded bg-gray-100 text-gray-300" :class="{ 'bg-yellow-100': status.lights > 0, 'text-yellow-500': status.lights > 0 }">
            <i class="uil uil-lightbulb-alt text-2xl"></i>
          </div>
          <div class="flex flex-col flex-grow ml-3 justify-center">
            <div class="text-sm text-gray-600 font-bold nunito">Automatic Lights</div>
            <div class="font-semibold uppercase text-gray-400 transition" :class="{ 'text-yellow-600': status.lights > 0 }">
              <span v-if="!('lights' in status)">OFFLINE</span>
              <span v-else-if="status.lights == 0">OFF</span>
              <span v-else-if="status.lights == 1">ON</span>
              <span v-else>AUTO</span>
            </div>
          </div>
        </div>
      </div>
    </div>

    <div class="container mx-auto my-2 max-w-screen-lg px-8 md:flex gap-2">
      <div class="flex-1">
        <h2 class="text-2xl font-semibold ml-3 mb-1">System Devices</h2>
        <div class="bg-white shadow-sm rounded">
          <p class="px-3 py-2">Within this section of the dashboard, you can see the devices that are currently attached to the system as well as the previously connected ones.</p>
          <table class="w-full text-sm">
            <thead>
              <tr class="bg-gray-200 text-gray-600">
                <th class="py-1 px-2 pl-3 font-semibold">Type</th>
                <th class="py-1 px-2 font-semibold">Device</th>
                <th class="py-1 px-1 font-semibold">Status</th>
              </tr>
            </thead>
            <tbody>
              <tr v-for="device, index in devices" :key="device.device_id" class="border-t border-gray-200 hover:bg-gray-100" :class="{'bg-gray-50': index%2==1}">
                <td class="py-2 px-2 pl-3 font-mono">{{ device.type }}</span></td>
                <td class="py-2 px-2 font-mono">{{ device.device_id }}</td>
                <td class="py-2 px-1">
                  <span v-if="device.online" class="-mt-1 px-1.5 py-px rounded-full bg-green-200 text-xs text-green-700 font-bold">Online</span>
                  <span v-else :title="device.last_seen">{{ device.last_seen | moment("add", "2 hours", "from") }}</span>
                </td>
              </tr>
            </tbody>
          </table>
        </div>
      </div>

      <div class="flex-1 md:p-0 pt-4">
        <h2 class="ml-3 mb-1 text-2xl font-semibold">Parking Lot Status</h2>
        <div class="px-3 py-2 bg-white shadow-sm rounded">
          <p>Empty parking slot indicators will be populated when new parking sensors are detected.</p>
          <div class="mt-6 flex items-baseline">
            <div class="grid grid-cols-12 w-full mr-4 text-center">
              <div class="text-2xl py-1 cursor-pointer" v-for="slot, index in slots" :key="'p'+index" @click="parkingSlotToggle(index, slot.mac_address)">
                <span v-if="slot.status == 'on'">
                  <i v-if="slot.busy" class="uil uil-times-circle text-red-500"></i>
                  <i v-else class="uil uil-parking-circle text-green-500"></i>
                </span>
                <span v-else>
                  <i v-if="slot.busy" class="uil uil-times-circle text-gray-400"></i>
                  <i v-else class="uil uil-parking-circle text-gray-400"></i>
                </span>
              </div>

              <div class="text-2xl py-1 text-gray-300" v-for="_, index in (12 - slots.length)" :key="'e'+index">
                &bull;
              </div>
            </div>
            <div class="flex-1 text-sm">
              <div class="whitespace-nowrap"><span class="inline-block rounded-full mb-px w-2 h-2 bg-green-500"></span> Available</div>
              <div class="whitespace-nowrap"><span class="inline-block rounded-full mb-px w-2 h-2 bg-red-500"></span> Occupied</div>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div><!-- /#app -->

  <div class="container mx-auto text-xs text-gray-500 tracking-wide text-center py-2">
    <p class="sm:inline-block">Marchetti / Vincenzi &ndash;  Laboratorio IoT@Unimib &ndash; A.A.2020/2021</p>
    <span class="hidden sm:inline-block"> &ndash; </span>
    <p class="sm:inline-block">Unicons by <a href="https://iconscout.com/" class="text-purple-500">Iconscout</a></p>
  </div>

  <script src="https://cdn.jsdelivr.net/npm/vue@2.6.12/dist/vue.js"></script>
  <!-- <script src="https://cdn.jsdelivr.net/npm/vue@2.6.12"></script> -->
  <script src="https://cdn.jsdelivr.net/npm/moment@2.29.1/moment.min.js"></script>
  <script src="https://cdn.jsdelivr.net/npm/vue-moment@4.1.0/dist/vue-moment.min.js"></script>
  <script>
    const ENDPOINT = '';
    Vue.use(vueMoment);
    const app = new Vue({
      el: '#app',
      data: {
        loading: false,
        status: {},
        devices: [],
        slots: [],
      },

      mounted () {
        this.fetchData()
        setInterval(this.fetchData, 10000);
      },

      methods: {
        async fetchData() {
          if (this.loading) {
            return;
          }

          try {
            this.loading = true;

            let response = await fetch(ENDPOINT+'/api/status');
            // const response = await fetch(ENDPOINT+'/data.json');

            const data = await response.json();
            this.status = data.status
            this.devices = data.devices
            this.slots = data.parking_slots_availability
          } catch {
            this.error = "Update failed"
          }

          this.loading = false;
        },

        async displayToggle() {
          if (this.loading) {
            return;
          }

          try {
            this.loading = true;

            const command = (!this.status.display) ? "on" : "off"
            const response = await fetch(ENDPOINT+'/api/display', {
              method: 'POST',
              body: JSON.stringify({ command: command })
            });

            this.status.display = !this.status.display
          } catch {
            this.error = "Update failed"
          }

          this.loading = false;
        },

        async lightsToggle() {
          if (this.loading) {
            return;
          }

          try {
            this.loading = true;

            const command = (this.status.lights+1) % 3;
            const response = await fetch(ENDPOINT+'/api/lights', {
              method: 'POST',
              body: JSON.stringify({ command: command })
            });

            // console.log((this.status.lights+1)%3);
            this.status.lights = (this.status.lights+1) % 3
          } catch {
            this.error = "Update failed"
          }

          this.loading = false;
        },

        async parkingSlotToggle(index, mac) {
          if (this.loading) {
            return;
          }

          try {
            this.loading = true;

            const command = (this.slots[index].status == "on") ? "off" : "on";
            const response = await fetch(ENDPOINT+'/api/parking-slots?device_id='+mac, {
              method: 'POST',
              body: JSON.stringify({ command: command })
            });

            this.$set(this.slots, index, {...this.slots[index], status: command});
          } catch {
            this.error = "Update failed"
          }

          this.loading = false;
        }
      }
    })
  </script>
</body>
</html>
)=====";

#endif
