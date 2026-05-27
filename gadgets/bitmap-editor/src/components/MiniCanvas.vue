<script setup lang="ts">
import { ref, onMounted, watch } from 'vue'
import type { Bitmap } from '../types'

const props = defineProps<{
  bitmap: Bitmap
}>()

const canvasRef = ref<HTMLCanvasElement | null>(null)
const maxPreview = 64

function draw() {
  const canvas = canvasRef.value
  if (!canvas) return
  const ctx = canvas.getContext('2d')
  if (!ctx) return

  const { width, height, pixels } = props.bitmap
  const scale = Math.max(1, Math.floor(Math.min(maxPreview / width, maxPreview / height)))

  canvas.width = width * scale
  canvas.height = height * scale

  ctx.fillStyle = '#ffffff'
  ctx.fillRect(0, 0, canvas.width, canvas.height)

  ctx.fillStyle = '#18181b'
  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      if (pixels[y][x]) {
        ctx.fillRect(x * scale, y * scale, scale, scale)
      }
    }
  }
}

onMounted(draw)
watch(() => props.bitmap, draw, { deep: true })
</script>

<template>
  <canvas ref="canvasRef" class="border border-zinc-200 rounded" />
</template>
