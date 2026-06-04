<template>
  <div class="flex flex-col gap-3">
    <textarea
      ref="textInput"
      v-model="rawText"
      class="w-full h-16 px-3 py-2 border border-gray-300 rounded text-sm font-mono resize-none"
      placeholder="Hello %41%42%43  or  %%  or  カ゛"
      @input="error = null"
      @keydown.enter.prevent="handleApply"
    />
    <div class="flex justify-center">
      <button
        class="px-6 py-1.5 bg-blue-500 text-white rounded text-sm hover:bg-blue-600 cursor-pointer select-none"
        @click="handleApply"
      >
        Apply
      </button>
    </div>
    <div
      v-if="error"
      class="bg-red-50 border border-red-200 text-red-700 text-sm rounded px-3 py-2 select-none"
    >
      {{ error }}
    </div>
    <div v-if="cString" class="flex items-start gap-2">
      <label class="text-xs text-gray-500 select-none mt-1 shrink-0">C string:</label>
      <div
        class="flex-1 bg-gray-100 border border-gray-200 rounded px-3 py-2 text-xs font-mono break-all select-all"
      >
        {{ cString }}
      </div>
    </div>
    <div
      class="overflow-auto border border-gray-200 rounded bg-gray-50 flex justify-center p-4"
      :style="{ minHeight: previewHeight + 'px' }"
    >
      <canvas
        ref="canvasRef"
        class="block"
      />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, nextTick } from 'vue'
import { getCharBitmap, getCharWidth } from '../utils/fontParser'
import { mapUnicodeToBytes, bytesToCString } from '../utils/fontMapping'

const props = defineProps<{
  data: Uint8Array
  bytesPerChar: number
  width: number
  height: number
  varWidth: boolean
}>()

const rawText = ref('')
const canvasRef = ref<HTMLCanvasElement>()
const textInput = ref<HTMLTextAreaElement>()
const error = ref<string | null>(null)
const cString = ref('')

const charSpacing = 1
const previewHeight = props.height * 2 + 16

function parseText(input: string): { bytes: number[]; errors: string[] } {
  const bytes: number[] = []
  const errors: string[] = []
  let i = 0

  while (i < input.length) {
    if (input[i] === '%') {
      if (i + 1 < input.length && input[i + 1] === '%') {
        bytes.push(0x25)
        i += 2
      } else if (i + 2 < input.length) {
        const hex = input.slice(i + 1, i + 3)
        const v = parseInt(hex, 16)
        if (!isNaN(v)) {
          bytes.push(v)
          i += 3
        } else {
          errors.push(`Invalid hex after % at position ${i}: "${hex}"`)
          i += 3
        }
      } else {
        errors.push(`Incomplete escape "%" at position ${i}`)
        i++
      }
    } else if (input[i] === '\r') {
      i++
    } else {
      const { bytes: mapped, error: mapErr } = mapUnicodeToBytes(input[i])
      if (mapErr) {
        errors.push(mapErr)
      } else {
        bytes.push(...mapped)
      }
      i++
    }
  }
  return { bytes, errors }
}

function draw(bytes: number[]) {
  const canvas = canvasRef.value
  if (!canvas) {
    error.value = 'Canvas not ready. Please try again.'
    return
  }
  const ctx = canvas.getContext('2d')
  if (!ctx) {
    error.value = 'Failed to get 2D context.'
    return
  }

  let totalW = 128
  if (bytes.length > 0) {
    if (props.varWidth) {
      let sum = 0
      for (const b of bytes) {
        if (b >= 0 && b < 256) {
          const cw = getCharWidth(props.data, props.bytesPerChar, b)
          sum += cw * 2 + charSpacing
        }
      }
      if (sum > 0) totalW = sum
    } else {
      const charW = props.width * 2 + charSpacing
      totalW = bytes.length * charW
    }
  }
  const totalH = props.height * 2

  canvas.width = totalW
  canvas.height = totalH

  ctx.clearRect(0, 0, canvas.width, canvas.height)

  let offsetX = 0

  for (let bi = 0; bi < bytes.length; bi++) {
    const code = bytes[bi]
    if (code < 0 || code >= 256) continue
    const bitmap = getCharBitmap(props.data, props.bytesPerChar, code, props.width, props.height, props.varWidth)

    const cw = props.varWidth ? getCharWidth(props.data, props.bytesPerChar, code) : props.width
    const renderWidth = cw * 2

    for (let row = 0; row < props.height; row++) {
      for (let col = 0; col < renderWidth / 2; col++) {
        if (bitmap[row][col]) {
          ctx.fillStyle = '#1f2937'
          ctx.fillRect(offsetX + col * 2, row * 2, 2, 2)
        }
      }
    }

    offsetX += renderWidth + charSpacing
  }
}

async function handleApply() {
  error.value = null
  cString.value = ''

  const { bytes, errors } = parseText(rawText.value)
  if (errors.length > 0) {
    error.value = errors.join('; ')
    return
  }

  cString.value = bytesToCString(bytes)

  await nextTick()
  draw(bytes)
}

function resizeCanvas() {
  if (!canvasRef.value) return
  canvasRef.value.width = 128
  canvasRef.value.height = props.height * 2
}

defineExpose({ focus: () => textInput.value?.focus() })

onMounted(() => {
  nextTick(resizeCanvas)
})
</script>
