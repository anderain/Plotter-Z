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
      <div v-if="varWidth" class="flex items-center gap-2">
        <label class="text-sm text-gray-600 select-none">Char Width:</label>
        <input
          v-model.number="localCharWidth"
          type="number"
          :min="1"
          :max="fontWidth"
          class="w-16 px-2 py-1 border border-gray-300 rounded text-sm font-mono"
          @input="onCharWidthInput"
        />
      </div>
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
  varWidth: boolean
  charWidth: number
  fontWidth: number
}>()

const emit = defineEmits<{
  'apply': [bitmap: boolean[][]]
  'apply-width': [width: number]
}>()

const canvasRef = ref<HTMLCanvasElement>()
const cellSize = 32
const isDrawing = ref(false)
const lastToggled = ref<{ row: number; col: number } | null>(null)

const canvasWidth = computed(() => props.width * cellSize)
const canvasHeight = computed(() => props.height * cellSize)

const workingBitmap = ref<boolean[][] | null>(null)
const hasChanges = ref(false)
const localCharWidth = ref(props.charWidth)

watch(() => props.charWidth, (v) => {
  localCharWidth.value = v
})

watch(() => props.bitmap, (newVal) => {
  if (newVal) {
    workingBitmap.value = cloneBitmap(newVal)
  } else {
    workingBitmap.value = null
  }
  hasChanges.value = false
})

function cloneBitmap(b: boolean[][]): boolean[][] {
  return b.map((row) => [...row])
}

function emptyBitmap(): boolean[][] {
  return Array.from({ length: props.height }, () => Array(props.width).fill(false))
}

function draw() {
  const canvas = canvasRef.value
  if (!canvas) return
  const ctx = canvas.getContext('2d')
  if (!ctx) return

  ctx.clearRect(0, 0, canvas.width, canvas.height)

  ctx.fillStyle = '#f9fafb'
  ctx.fillRect(0, 0, canvas.width, canvas.height)

  if (props.varWidth) {
    const cw = localCharWidth.value
    if (cw < props.width) {
      ctx.fillStyle = '#fce4ec'
      ctx.fillRect(cw * cellSize, 0, (props.width - cw) * cellSize, props.height * cellSize)
    }
  }

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

  if (props.varWidth) {
    const cw = localCharWidth.value
    if (cw > 0 && cw < props.width) {
      ctx.strokeStyle = '#ef5350'
      ctx.lineWidth = 2
      const x = cw * cellSize
      ctx.beginPath()
      ctx.moveTo(x, 0)
      ctx.lineTo(x, props.height * cellSize)
      ctx.stroke()
    }
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

function onCharWidthInput() {
  const w = Math.max(1, Math.min(props.fontWidth, localCharWidth.value))
  localCharWidth.value = w
  emit('apply-width', w)
}

function clearAll() {
  if (!workingBitmap.value) return
  const wasEmpty = workingBitmap.value.every((row) => row.every((p) => !p))
  workingBitmap.value = emptyBitmap()
  hasChanges.value = !wasEmpty
  draw()
}

defineExpose({ clearAll })

watch(() => [workingBitmap.value, localCharWidth.value], () => {
  draw()
}, { deep: true })
</script>
