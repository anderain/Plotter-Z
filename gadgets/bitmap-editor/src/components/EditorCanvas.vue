<script setup lang="ts">
import { ref, onMounted, watch } from 'vue'

const props = defineProps<{
  pixels: boolean[][]
  canvasVersion: number
}>()

const emit = defineEmits<{
  cellClick: [x: number, y: number]
}>()

const canvasRef = ref<HTMLCanvasElement | null>(null)
const pixelSize = 24

let isDrawing = false
let lastPixel = ''

function draw() {
  const canvas = canvasRef.value
  if (!canvas) return
  const ctx = canvas.getContext('2d')
  if (!ctx) return

  const h = props.pixels.length
  const w = h > 0 ? props.pixels[0].length : 0

  canvas.width = w * pixelSize
  canvas.height = h * pixelSize

  ctx.fillStyle = '#ffffff'
  ctx.fillRect(0, 0, canvas.width, canvas.height)

  ctx.fillStyle = '#18181b'
  for (let y = 0; y < h; y++) {
    for (let x = 0; x < w; x++) {
      if (props.pixels[y][x]) {
        ctx.fillRect(x * pixelSize + 1, y * pixelSize + 1, pixelSize - 2, pixelSize - 2)
      }
    }
  }

  ctx.strokeStyle = '#d4d4d8'
  ctx.lineWidth = 0.5
  for (let x = 0; x <= w; x++) {
    ctx.beginPath()
    ctx.moveTo(x * pixelSize, 0)
    ctx.lineTo(x * pixelSize, h * pixelSize)
    ctx.stroke()
  }
  for (let y = 0; y <= h; y++) {
    ctx.beginPath()
    ctx.moveTo(0, y * pixelSize)
    ctx.lineTo(w * pixelSize, y * pixelSize)
    ctx.stroke()
  }
}

function pixelFromEvent(e: MouseEvent): { x: number; y: number } | null {
  const canvas = canvasRef.value
  if (!canvas) return null
  const rect = canvas.getBoundingClientRect()
  const x = Math.floor((e.clientX - rect.left) / pixelSize)
  const y = Math.floor((e.clientY - rect.top) / pixelSize)
  const h = props.pixels.length
  const w = h > 0 ? props.pixels[0].length : 0
  if (x < 0 || x >= w || y < 0 || y >= h) return null
  return { x, y }
}

function handleMouseDown(e: MouseEvent) {
  const pos = pixelFromEvent(e)
  if (!pos) return
  isDrawing = true
  lastPixel = `${pos.x},${pos.y}`
  emit('cellClick', pos.x, pos.y)
}

function handleMouseMove(e: MouseEvent) {
  if (!isDrawing) return
  const pos = pixelFromEvent(e)
  if (!pos) return
  const key = `${pos.x},${pos.y}`
  if (key === lastPixel) return
  lastPixel = key
  emit('cellClick', pos.x, pos.y)
}

function handleMouseUp() {
  isDrawing = false
}

onMounted(draw)
watch(() => props.canvasVersion, draw)
watch(() => props.pixels, draw, { deep: true })
</script>

<template>
  <canvas
    ref="canvasRef"
    class="rounded cursor-crosshair select-none shadow-sm"
    @mousedown="handleMouseDown"
    @mousemove="handleMouseMove"
    @mouseup="handleMouseUp"
    @mouseleave="handleMouseUp"
  />
</template>
