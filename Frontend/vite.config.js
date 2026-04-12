import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

export default defineConfig({
  plugins: [vue()],
  server: {
    proxy: {
      '/frontend': {
        target: 'http://10.176.101.126:8080',
        changeOrigin: true
      }
    }
  }
})
