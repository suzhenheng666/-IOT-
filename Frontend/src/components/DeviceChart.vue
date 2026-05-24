<!-- src/components/DeviceChart.vue -->
<template>
  <div class="chart-container">
    <div class="chart-title">{{ title }}</div>
    <div class="chart-wrapper">
      <v-chart
        class="chart"
        :option="chartOption"
        :loading="loading"
        autoresize
      />
    </div>
  </div>
</template>

<script>
import axios from 'axios'
import { ref, watch, onMounted } from 'vue'



export default {
  name: 'DeviceChart',
  props: {
    deviceId: {
      type: String,
      required: true
    },
    chartType: {
      type: String,
      default: 'temperature', // 'temperature' 或 'altitude'
      validator: value => ['temperature', 'altitude'].includes(value)
    }
  },
  setup(props) {
    const API_CONFIG = {
      baseURL: '',   // ✅ 关键：不要写 http://192...
      endpoints: {
        temperatureHistory: '/frontend/device/temperature',
        humidityHistory: '/frontend/device/humidity'
      }
    }


    const request = axios.create({
      baseURL: API_CONFIG.baseURL,
      timeout: 5000
    })

    const loading = ref(false)
    const chartData = ref([])
    const chartOption = ref({})

    const title = ref(props.chartType === 'temperature' ? '温度变化' : '高度变化')
    const yAxisName = ref(props.chartType === 'temperature' ? '温度(°C)' : '高度(m)')

    const updateChartOption = () => {
      const times = chartData.value.map(item => item.time)
      const values = chartData.value.map(item => 
        props.chartType === 'temperature' ? item.temperature : item.altitude
      )

      chartOption.value = {
        title: {
          text: `${props.deviceId} - ${title.value}`,
          left: 'center'
        },
        tooltip: {
          trigger: 'axis',
          formatter: '{b0}<br />' + yAxisName.value + ': {c0}'
        },
        grid: {
          left: '3%',
          right: '4%',
          bottom: '3%',
          containLabel: true
        },
        xAxis: {
          type: 'category',
          data: times,
          name: '时间',
          axisLabel: {
            rotate: 45
          }
        },
        yAxis: {
          type: 'value',
          name: yAxisName.value
        },
        series: [
          {
            name: title.value,
            type: 'line',
            data: values,
            smooth: true,
            lineStyle: {
              width: 3
            },
            itemStyle: {
              color: props.chartType === 'temperature' ? '#ff6b6b' : '#4ecdc4'
            },
            areaStyle: {
              color: props.chartType === 'temperature' 
                ? {
                    type: 'linear',
                    x: 0, y: 0, x2: 0, y2: 1,
                    colorStops: [
                      { offset: 0, color: 'rgba(255, 107, 107, 0.5)' },
                      { offset: 1, color: 'rgba(255, 107, 107, 0.1)' }
                    ]
                  }
                : {
                    type: 'linear',
                    x: 0, y: 0, x2: 0, y2: 1,
                    colorStops: [
                      { offset: 0, color: 'rgba(78, 205, 196, 0.5)' },
                      { offset: 1, color: 'rgba(78, 205, 196, 0.1)' }
                    ]
                  }
            }
          }
        ]
      }
    }

    // 模拟API数据 - 实际使用时请调用真实的API
    // const fetchData = () => {
    //   loading.value = true
      
    //   // 模拟API延迟
    //   setTimeout(() => {
    //     // 模拟数据
    //     const baseValue = props.chartType === 'temperature' ? 20 : 10
    //     chartData.value = Array.from({ length: 10 }, (_, i) => {
    //       const minute = 25 - i * 2
    //       const time = `14:${minute.toString().padStart(2, '0')}`
    //       const value = baseValue + Math.random() * 5 - 2.5
          
    //       return {
    //         time,
    //         [props.chartType === 'temperature' ? 'temperature' : 'altitude']: 
    //           Number(value.toFixed(1))
    //       }
    //     }).reverse()

    //     updateChartOption()
    //     loading.value = false
    //   }, 500)
    // }

    const getTemperatureHistory = (deviceId) => {
      return request.get(API_CONFIG.endpoints.temperatureHistory, {
        params: { device_id: deviceId }
      })
    }

    const getHumidityHistory = (deviceId) => {
      return request.get(API_CONFIG.endpoints.humidityHistory, {
        params: { device_id: deviceId }
      })
    }


    const fetchData = async () => {
      loading.value = true
      try {
        let res

        if (props.chartType === 'temperature') {
          res = await getTemperatureHistory(props.deviceId)
        } else {
          // altitude 实际走湿度接口
          res = await getHumidityHistory(props.deviceId)
        }

        if (res.data.code !== 0) {
          console.error('接口返回错误:', res.data.msg)
          return
        }

        chartData.value = res.data.data.map(item => ({
          time: item.time,
          temperature: item.temperature
            ? Number(item.temperature)
            : undefined,
          altitude: item.humidity
            ? Number(item.humidity)
            : undefined
        }))

        updateChartOption()
      } catch (err) {
        console.error('请求失败:', err)
      } finally {
        loading.value = false
      }
    }


    onMounted(() => {
      fetchData()
    })

    watch(() => props.chartType, () => {
      title.value = props.chartType === 'temperature' ? '温度变化' : '高度变化'
      yAxisName.value = props.chartType === 'temperature' ? '温度(°C)' : '高度(m)'
      fetchData()
    })

    return {
      loading,
      chartOption,
      title
    }
  }
}
</script>

<style scoped>
.chart-container {
  width: 100%;
  height: 400px;
  padding: 20px;
  background: #fff;
  border-radius: 8px;
  box-shadow: 0 2px 12px 0 rgba(0, 0, 0, 0.1);
}

.chart-title {
  font-size: 18px;
  font-weight: bold;
  margin-bottom: 20px;
  color: #333;
  text-align: center;
}

.chart-wrapper {
  width: 100%;
  height: calc(100% - 40px);
}

.chart {
  width: 100%;
  height: 100%;
}
</style>