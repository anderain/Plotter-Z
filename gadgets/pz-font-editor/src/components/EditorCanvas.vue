<template>
  <div class="flex flex-col items-center p-4 gap-3">
    <div v-if="!bitmap" class="flex items-center justify-center h-full text-gray-400">
      No character selected
    </div>
    <template v-else>
      <span class="text-sm text-gray-500 font-mono">
        Char 0x{{ selectedIndex.toString(16).toUpperCase().padStart(2, '0') }} ({{ selectedIndex }})
      </span>
      <canvas
        ref="canvasRef"
        :width="canvasWidth"
        :height="canvasHeight"
        class="border border-gray-400"
        :class="tool === 'brush' ? 'cursor-crosshair' : 'cursor-cell'"
        @mousedown="handleMouseDown"
        @mousemove="handleMouseMove"
        @mouseup="handleMouseUp"
        @mouseleave="handleMouseUp"
      />
      <button
        class="px-4 py-1.5 bg-blue-500 text-white rounded text-sm hover:bg-blue-600 cursor-pointer disabled:opacity-50 disabled:cursor-not-allowed"
        :disabled="!hasChanges"
        @click="handleApply"
      >
        Apply
      </button>
    </template>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, computed } from 'vue'

const props = defineProps<{
  bitmap: boolean[][] | null
  width: number
  height: number
  selectedIndex: number
  tool: 'brush' | 'eraser'
}>()

const emit = defineEmits<{
  'apply': [bitmap: boolean[][]]
}>()

const canvasRef = ref<HTMLCanvasElement>()
const cellSize = 32
const isDrawing = ref(false)
const lastToggled = ref<{ row: number; col: number } | null>(null)

const canvasWidth = computed(() => props.width * cellSize)
const canvasHeight = computed(() => props.height * cellSize)

const workingBitmap = ref<boolean[][] | null>(null)
const hasChanges = ref(false)

function cloneBitmap(b: boolean[][]): boolean[][] {
  return b.map((row) => [...row])
}

function emptyBitmap(): boolean[][] {
  return Array.from({ length: props.height }, () => Array(props.width).fill(false))
}

watch(() => props.bitmap, (newVal) => {
  if (newVal) {
    workingBitmap.value = cloneBitmap(newVal)
  } else {
    workingBitmap.value = null
  }
  hasChanges.value = false
})

function draw() {
  const canvas = canvasRef.value
  if (!canvas) return
  const ctx = canvas.getContext('2d')
  if (!ctx) return

  ctx.clearRect(0, 0, canvas.width, canvas.height)

  ctx.fillStyle = '#f9fafb'
  ctx.fillRect(0, 0, canvas.width, canvas.height)

  const bm = workingBitmap.value
  if (!bm) return

  for (let row = 0; row < props.height; row++) {
    for (let col = 0; col < props.width; col++) {
      const x = col * cellSize
      const y = row * cellSize
      if (bm[row][col]) {
        ctx.fillStyle = '#1f2937'
        ctx.fillRect(x, y, cellSize, cellSize)
      }
    }
  }

  ctx.strokeStyle = '#d1d5db'
  ctx.lineWidth = 1
  for (let row = 0; row <= props.height; row++) {
    ctx.beginPath()
    ctx.moveTo(0, row * cellSize)
    ctx.lineTo(props.width * cellSize, row * cellSize)
    ctx.stroke()
  }
  for (let col = 0; col <= props.width; col++) {
    ctx.beginPath()
    ctx.moveTo(col * cellSize, 0)
    ctx.lineTo(col * cellSize, props.height * cellSize)
    ctx.stroke()
  }
}

function getPixelFromEvent(e: MouseEvent): { row: number; col: number } | null {
  const canvas = canvasRef.value
  if (!canvas) return null
  const rect = canvas.getBoundingClientRect()
  const x = e.clientX - rect.left
  const y = e.clientY - rect.top
  const col = Math.floor(x / cellSize)
  const row = Math.floor(y / cellSize)
  if (row < 0 || row >= props.height || col < 0 || col >= props.width) return null
  return { row, col }
}

function setPixel(row: number, col: number) {
  if (!workingBitmap.value) return
  const newVal = props.tool === 'brush'
  if (workingBitmap.value[row][col] !== newVal) {
    workingBitmap.value[row][col] = newVal
    workingBitmap.value = cloneBitmap(workingBitmap.value)
    hasChanges.value = true
    draw()
  }
}

function handleMouseDown(e: MouseEvent) {
  isDrawing.value = true
  const pos = getPixelFromEvent(e)
  if (pos) {
    setPixel(pos.row, pos.col)
    lastToggled.value = pos
  }
}

function handleMouseMove(e: MouseEvent) {
  if (!isDrawing.value) return
  const pos = getPixelFromEvent(e)
  if (pos && (pos.row !== lastToggled.value?.row || pos.col !== lastToggled.value?.col)) {
    setPixel(pos.row, pos.col)
    lastToggled.value = pos
  }
}

function handleMouseUp() {
  isDrawing.value = false
  lastToggled.value = null
}

function handleApply() {
  if (workingBitmap.value) {
    emit('apply', cloneBitmap(workingBitmap.value))
    hasChanges.value = false
  }
}

function clearAll() {
  if (!workingBitmap.value) return
  const wasEmpty = workingBitmap.value.every((row) => row.every((p) => !p))
  workingBitmap.value = emptyBitmap()
  hasChanges.value = !wasEmpty
  draw()
}

defineExpose({ clearAll })

watch(() => workingBitmap.value, draw, { deep: true })
</script>
