// src/api/index.js
import request from '../utils/request'

// 获取所有设备最近一次监测数据
export function getLatestDevices() {
  return request({
    url: '/api/v1/devices/latest',
    method: 'get'
  })
}

// 获取设备温度变化
export function getDeviceTemperature(deviceId) {
  return request({
    url: `/api/v1/device/temperature?device_id=${deviceId}`,
    method: 'get'
  })
}

// 获取设备高度变化
export function getDeviceAltitude(deviceId) {
  return request({
    url: `/api/v1/device/altitude?device_id=${deviceId}`,
    method: 'get'
  })
}