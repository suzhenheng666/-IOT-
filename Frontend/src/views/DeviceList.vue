<!-- src/views/DeviceList.vue -->
<template>
  <div class="device-list-container">
    <div class="header">
      <h1 class="title">IoT 监测系统</h1>
      <div class="header-actions">
        <button @click="refreshData" class="refresh-btn">
          <span v-if="!loading">刷新数据</span>
          <span v-else>刷新中...</span>
        </button>
        <div class="stats">
          <span>设备总数: {{ devices.length }}</span>
          <span>在线设备: {{ onlineCount }}</span>
        </div>
      </div>
    </div>

    <div class="table-container">
      <div class="table-responsive">
        <table class="device-table">
          <thead>
            <tr>
              <th>设备ID</th>
              <th>设备名称</th>
              <th>经度</th>
              <th>纬度</th>
              <th>温度(°C)</th>
              <th>高度(m)</th>
              <th>障碍距离(m)</th>
              <th>最后更新时间</th>
              <th>操作</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="device in devices" :key="device.device_id" :class="{ 'offline': isOffline(device) }">
              <td>{{ device.device_id }}</td>
              <td>{{ device.name }}</td>
              <td>{{ device.longitude.toFixed(4) }}</td>
              <td>{{ device.latitude.toFixed(4) }}</td>
              <td :class="getTemperatureClass(device.temperature)">
                {{ device.temperature }}
              </td>
              <td>{{ device.altitude }}</td>
              <td :class="getDistanceClass(device.obstacle_distance)">
                {{ device.obstacle_distance }}
              </td>
              <td>{{ formatTime(device.timestamp) }}</td>
              <td>
                <div class="action-buttons">
                  <button 
                    @click="showChart(device.device_id, 'temperature')"
                    class="btn btn-primary"
                  >
                    温度图表
                  </button>
                  <button 
                    @click="showChart(device.device_id, 'altitude')"
                    class="btn btn-secondary"
                  >
                    高度图表
                  </button>
                </div>
              </td>
            </tr>
            <tr v-if="devices.length === 0">
              <td colspan="9" class="no-data">
                {{ loading ? '加载中...' : '暂无数据' }}
              </td>
            </tr>
          </tbody>
        </table>
      </div>
    </div>

    <!-- 图表模态框 -->
    <div v-if="showChartModal" class="modal-overlay" @click.self="closeChart">
      <div class="modal-content">
        <div class="modal-header">
          <h3>设备监测图表 - {{ currentDeviceId }}</h3>
          <button @click="closeChart" class="close-btn">&times;</button>
        </div>
        <div class="modal-body">
          <div class="chart-tabs">
            <button 
              @click="currentChartType = 'temperature'"
              :class="{ 'active': currentChartType === 'temperature' }"
              class="tab-btn"
            >
              温度变化
            </button>
            <button 
              @click="currentChartType = 'altitude'"
              :class="{ 'active': currentChartType === 'altitude' }"
              class="tab-btn"
            >
              高度变化
            </button>
          </div>
          <DeviceChart 
            v-if="showChartModal"
            :device-id="currentDeviceId"
            :chart-type="currentChartType"
            class="modal-chart"
          />
        </div>
      </div>
    </div>
  </div>
</template>

<script>
import { ref, onMounted, computed } from 'vue'
import axios from 'axios'
import DeviceChart from '../components/DeviceChart.vue'


export default {
  name: 'DeviceList',
  components: {
    DeviceChart
  },
  setup() {
    const API_CONFIG = {
      baseURL: '',
      endpoints: {
        latestDevices: '/frontend/devices/latest'
      }
    }

    const request = axios.create({
      baseURL: API_CONFIG.baseURL,
      timeout: 5000
    })

    const devices = ref([])
    const loading = ref(false)
    const showChartModal = ref(false)
    const currentDeviceId = ref('')
    const currentChartType = ref('temperature')

    // 模拟API数据 - 实际使用时请调用真实的API
    // const fetchDevices = async () => {
    //   loading.value = true
    //   try {
    //     // 模拟API调用
    //     await new Promise(resolve => setTimeout(resolve, 800))
        
    //     // 模拟数据
    //     devices.value = Array.from({ length: 8 }, (_, i) => {
    //       const id = `dev${(i + 1).toString().padStart(3, '0')}`
    //       const baseTemp = 20 + Math.random() * 10
    //       const baseAlt = 10 + Math.random() * 10
          
    //       return {
    //         device_id: id,
    //         name: `设备${i + 1}`,
    //         longitude: 121.1 + Math.random() * 0.2,
    //         latitude: 31.2 + Math.random() * 0.2,
    //         temperature: Number(baseTemp.toFixed(1)),
    //         altitude: Number(baseAlt.toFixed(1)),
    //         obstacle_distance: Number((Math.random() * 2).toFixed(2)),
    //         timestamp: new Date(Date.now() - Math.random() * 600000).toISOString().replace('T', ' ').substr(0, 19)
    //       }
    //     })
    //   } catch (error) {
    //     console.error('获取设备数据失败:', error)
    //   } finally {
    //     loading.value = false
    //   }
    // }

    const fetchDevices = async () => {
      loading.value = true
      try {
        const res = await request.get(API_CONFIG.endpoints.latestDevices)

        if (res.data.code !== 0) {
          console.error('latest 接口返回错误:', res.data.msg)
          return
        }

        // 🔑 字段对齐就在这里做
        devices.value = res.data.data.map(item => ({
          device_id: item.device_id,
          name: item.device_id,                 // 后端没给 name，先用 id 顶
          longitude: Number(item.longitude),
          latitude: Number(item.latitude),
          temperature: Number(item.temperature),
          altitude: Number(item.humidity),      // 👈 humidity → altitude
          obstacle_distance: Number(item.obstacle_distance),
          timestamp: item.timestamp
        }))

      } catch (error) {
        console.error('获取设备数据失败:', error)
      } finally {
        loading.value = false
      }
    }


    const refreshData = () => {
      fetchDevices()
    }

    const showChart = (deviceId, chartType) => {
      currentDeviceId.value = deviceId
      currentChartType.value = chartType
      showChartModal.value = true
    }

    const closeChart = () => {
      showChartModal.value = false
    }

    const formatTime = (time) => {
      return time ? time.replace('T', ' ').substr(11, 8) : '--:--:--'
    }

    const getTemperatureClass = (temp) => {
      if (temp > 28) return 'high-temp'
      if (temp < 18) return 'low-temp'
      return ''
    }

    const getDistanceClass = (distance) => {
      if (distance < 0.5) return 'danger-distance'
      if (distance < 1) return 'warning-distance'
      return ''
    }

    const isOffline = (device) => {
      const updateTime = new Date(device.timestamp.replace(' ', 'T')).getTime()
      return Date.now() - updateTime > 300000 // 5分钟未更新视为离线
    }

    const onlineCount = computed(() => {
      return devices.value.filter(device => !isOffline(device)).length
    })

    onMounted(() => {
      fetchDevices()
    })

    return {
      devices,
      loading,
      showChartModal,
      currentDeviceId,
      currentChartType,
      onlineCount,
      refreshData,
      showChart,
      closeChart,
      formatTime,
      getTemperatureClass,
      getDistanceClass,
      isOffline
    }
  }
}
</script>

<style scoped>
.device-list-container {
  padding: 20px;
  max-width: 1400px;
  margin: 0 auto;
  font-family: 'Arial', sans-serif;
}

.header {
  margin-bottom: 30px;
  padding-bottom: 15px;
  border-bottom: 2px solid #e8e8e8;
}

.title {
  font-size: 28px;
  font-weight: bold;
  color: #1890ff;
  margin: 0 0 15px 0;
  text-align: center;
}

.header-actions {
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 15px;
}

.refresh-btn {
  padding: 8px 20px;
  background: #1890ff;
  color: white;
  border: none;
  border-radius: 4px;
  cursor: pointer;
  font-size: 14px;
  transition: background-color 0.3s;
}

.refresh-btn:hover {
  background: #40a9ff;
}

.refresh-btn:disabled {
  background: #d9d9d9;
  cursor: not-allowed;
}

.stats {
  display: flex;
  gap: 20px;
  font-size: 14px;
  color: #666;
}

.stats span {
  padding: 5px 10px;
  background: #f5f5f5;
  border-radius: 4px;
}

.table-container {
  background: white;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  overflow: hidden;
}

.table-responsive {
  overflow-x: auto;
}

.device-table {
  width: 100%;
  border-collapse: collapse;
  min-width: 1200px;
}

.device-table th {
  background: #fafafa;
  padding: 16px;
  text-align: left;
  font-weight: 600;
  color: #333;
  border-bottom: 1px solid #e8e8e8;
  white-space: nowrap;
}

.device-table td {
  padding: 16px;
  border-bottom: 1px solid #e8e8e8;
  color: #666;
}

.device-table tbody tr:hover {
  background: #f6faff;
}

.device-table tbody tr.offline {
  background: #f9f9f9;
  color: #999;
}

.device-table tbody tr.offline:hover {
  background: #f0f0f0;
}

.high-temp {
  color: #ff4d4f;
  font-weight: bold;
}

.low-temp {
  color: #1890ff;
  font-weight: bold;
}

.danger-distance {
  color: #ff4d4f;
  font-weight: bold;
}

.warning-distance {
  color: #faad14;
  font-weight: bold;
}

.action-buttons {
  display: flex;
  gap: 8px;
  flex-wrap: wrap;
}

.btn {
  padding: 6px 12px;
  border: none;
  border-radius: 4px;
  cursor: pointer;
  font-size: 12px;
  transition: all 0.3s;
  white-space: nowrap;
}

.btn-primary {
  background: #1890ff;
  color: white;
}

.btn-primary:hover {
  background: #40a9ff;
}

.btn-secondary {
  background: #52c41a;
  color: white;
}

.btn-secondary:hover {
  background: #73d13d;
}

.no-data {
  text-align: center;
  color: #999;
  padding: 40px !important;
}

/* 模态框样式 */
.modal-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(0, 0, 0, 0.5);
  display: flex;
  justify-content: center;
  align-items: center;
  z-index: 1000;
  padding: 20px;
}

.modal-content {
  background: white;
  border-radius: 8px;
  width: 90%;
  max-width: 1000px;
  max-height: 90vh;
  overflow-y: auto;
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.15);
}

.modal-header {
  padding: 20px;
  border-bottom: 1px solid #e8e8e8;
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.modal-header h3 {
  margin: 0;
  color: #333;
}

.close-btn {
  background: none;
  border: none;
  font-size: 24px;
  cursor: pointer;
  color: #999;
  line-height: 1;
  padding: 0;
  width: 30px;
  height: 30px;
  display: flex;
  align-items: center;
  justify-content: center;
}

.close-btn:hover {
  color: #333;
  background: #f5f5f5;
  border-radius: 50%;
}

.modal-body {
  padding: 20px;
}

.chart-tabs {
  display: flex;
  gap: 10px;
  margin-bottom: 20px;
  padding-bottom: 10px;
  border-bottom: 1px solid #e8e8e8;
}

.tab-btn {
  padding: 8px 20px;
  background: #f5f5f5;
  border: none;
  border-radius: 4px;
  cursor: pointer;
  font-size: 14px;
  color: #666;
  transition: all 0.3s;
}

.tab-btn:hover {
  background: #e8e8e8;
}

.tab-btn.active {
  background: #1890ff;
  color: white;
}

.modal-chart {
  height: 400px;
}

@media (max-width: 768px) {
  .device-list-container {
    padding: 10px;
  }
  
  .header-actions {
    flex-direction: column;
    align-items: stretch;
  }
  
  .action-buttons {
    flex-direction: column;
  }
  
  .modal-content {
    width: 95%;
  }
}
</style>