<template>
  <div class="flex flex-col h-full">
    <div class="px-3 py-2 text-sm text-gray-500 border-b border-gray-200 flex-shrink-0">
      {{ charCount }} characters
    </div>
    <div class="flex-1 overflow-auto p-2">
      <div class="grid grid-cols-8 gap-1">
        <div
          v-for="idx in charCount"
          :key="idx - 1"
          class="flex flex-col items-center cursor-pointer border-2 rounded py-0.5"
          :class="(idx - 1) === selectedIndex
            ? 'border-blue-500 bg-blue-50'
            : 'border-transparent hover:border-gray-300'"
          @click="$emit('select', idx - 1)"
        >
          <span class="text-[10px] text-gray-500 w-full font-mono leading-tight text-left">
            {{ formatTitle(idx - 1) }}
          </span>
          <canvas
            :ref="(el) => setCanvasRef(idx - 1, el as HTMLCanvasElement)"
            :width="cellW"
            :height="cellH"
            class="block"
          />
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { watch, onMounted, nextTick, computed } from 'vue'
import { getCharBitmap } from '../utils/fontParser'

const props = defineProps<{
  data: Uint8Array | null
  bytesPerChar: number
  width: number
  height: number
  charCount: number
  selectedIndex: number
  dataVersion: number
}>()

defineEmits<{
  'select': [index: number]
}>()

const pixelSize = 2
const cellW = computed(() => props.width * pixelSize)
const cellH = computed(() => props.height * pixelSize)

const canvasRefs = new Map<number, HTMLCanvasElement>()

function setCanvasRef(index: number, el: HTMLCanvasElement) {
  if (el) {
    canvasRefs.set(index, el)
  }
}

function formatTitle(index: number): string {
  const hex = '0x' + index.toString(16).toUpperCase().padStart(2, '0')
  if (index >= 0x20 && index <= 0x7E) {
    return hex + ' "' + String.fromCharCode(index) + '"'
  }
  return hex
}

function drawChar(index: number) {
  const canvas = canvasRefs.get(index)
  if (!canvas || !props.data) return
  const ctx = canvas.getContext('2d')
  if (!ctx) return

  const w = cellW.value
  const h = cellH.value
  if (canvas.width !== w) canvas.width = w
  if (canvas.height !== h) canvas.height = h

  ctx.clearRect(0, 0, w, h)
  const bitmap = getCharBitmap(props.data, props.bytesPerChar, index, props.width, props.height)

  for (let row = 0; row < props.height; row++) {
    for (let col = 0; col < props.width; col++) {
      if (bitmap[row][col]) {
        ctx.fillStyle = '#1f2937'
        ctx.fillRect(col * pixelSize, row * pixelSize, pixelSize, pixelSize)
      }
    }
  }
}

function drawAll() {
  for (let i = 0; i < props.charCount; i++) {
    drawChar(i)
  }
}

watch(() => props.dataVersion, () => {
  nextTick(drawAll)
})

watch([() => props.width, () => props.height], () => {
  nextTick(drawAll)
})

onMounted(() => {
  nextTick(drawAll)
})
</script>
