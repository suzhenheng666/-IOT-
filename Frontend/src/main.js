// src/main.js
import { createApp } from 'vue'
import App from './App.vue'

// 导入方式修改
import * as ECharts from 'echarts'
import VueECharts from 'vue-echarts'

const app = createApp(App)

// 注册为全局组件
app.component('v-chart', VueECharts)

app.mount('#app')