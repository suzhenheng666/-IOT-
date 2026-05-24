<!-- src/App.vue -->
<template>
  <DeviceList />
</template>

<script>
import DeviceList from './views/DeviceList.vue'

export default {
  name: 'App',
  components: {
    DeviceList
  }
}
</script>

<style>
* {
  margin: 0;
  padding: 0;
  box-sizing: border-box;
}

body {
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
  background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
  min-height: 100vh;
}

#app {
  min-height: 100vh;
}
</style>