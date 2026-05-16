<template>
  <div class="w-full px-4 py-2 bg-gray-100 border-b border-gray-300">
    <div class="max-w-6xl mx-auto flex items-center gap-2">
      <button
        class="inline-flex items-center gap-1 px-3 py-1.5 bg-white border border-gray-400 rounded text-sm hover:bg-gray-50 cursor-pointer select-none"
        @click="openFile"
      >
        <FolderOpen :size="16" />
        Open
      </button>
      <button
        class="inline-flex items-center gap-1 px-3 py-1.5 bg-white border border-gray-400 rounded text-sm hover:bg-gray-50 cursor-pointer select-none disabled:opacity-50 disabled:cursor-not-allowed"
        :disabled="!hasFont"
        @click="handleExport"
      >
        <Download :size="16" />
        Export
      </button>

      <div class="w-px h-6 bg-gray-300 mx-1" />

      <button
        class="inline-flex items-center gap-1 px-3 py-1.5 rounded text-sm cursor-pointer border select-none"
        :class="tool === 'brush'
          ? 'bg-blue-500 text-white border-blue-500'
          : 'bg-white border-gray-400 hover:bg-gray-50'"
        :disabled="!hasFont"
        @click="$emit('set-tool', 'brush')"
      >
        <Brush :size="16" />
        Brush
      </button>
      <button
        class="inline-flex items-center gap-1 px-3 py-1.5 rounded text-sm cursor-pointer border select-none"
        :class="tool === 'eraser'
          ? 'bg-blue-500 text-white border-blue-500'
          : 'bg-white border-gray-400 hover:bg-gray-50'"
        :disabled="!hasFont"
        @click="$emit('set-tool', 'eraser')"
      >
        <Eraser :size="16" />
        Eraser
      </button>
      <button
        class="inline-flex items-center gap-1 px-3 py-1.5 bg-white border border-gray-400 rounded text-sm hover:bg-gray-50 cursor-pointer select-none disabled:opacity-50 disabled:cursor-not-allowed"
        :disabled="!hasFont"
        @click="$emit('clear')"
      >
        <Trash2 :size="16" />
        Clear
      </button>

      <div class="w-px h-6 bg-gray-300 mx-1" />

      <button
        class="inline-flex items-center gap-1 px-3 py-1.5 bg-white border border-gray-400 rounded text-sm hover:bg-gray-50 cursor-pointer select-none disabled:opacity-50 disabled:cursor-not-allowed"
        :disabled="!hasFont"
        @click="openDialog('swap')"
      >
        <ArrowLeftRight :size="16" />
        Swap
      </button>
      <button
        class="inline-flex items-center gap-1 px-3 py-1.5 bg-white border border-gray-400 rounded text-sm hover:bg-gray-50 cursor-pointer select-none disabled:opacity-50 disabled:cursor-not-allowed"
        :disabled="!hasFont"
        @click="openDialog('duplicate')"
      >
        <Copy :size="16" />
        Duplicate
      </button>

      <div class="w-px h-6 bg-gray-300 mx-1" />

      <button
        class="inline-flex items-center gap-1 px-3 py-1.5 bg-white border border-gray-400 rounded text-sm hover:bg-gray-50 cursor-pointer select-none disabled:opacity-50 disabled:cursor-not-allowed"
        :disabled="!hasFont"
        @click="openPreview"
      >
        <Eye :size="16" />
        Preview
      </button>

      <span v-if="hasFont" class="text-sm text-gray-600 ml-2 select-none">
        {{ config?.font_name }} ({{ config?.font_width }}×{{ config?.font_height }})
      </span>
    </div>
  </div>

  <input
    ref="fileInput"
    type="file"
    accept=".h"
    class="hidden"
    @change="handleFileSelect"
  />

  <Dialog
    :open="dialogMode === 'swap'"
    title="Swap Characters"
    @confirm="handleSwapConfirm"
    @cancel="closeDialog"
  >
    <div class="flex items-center gap-3">
      <label class="text-sm text-gray-600 select-none w-16">Char A:</label>
      <input
        ref="swapInputA"
        v-model="hexA"
        class="flex-1 px-2 py-1 border border-gray-300 rounded text-sm font-mono select-none"
        placeholder="0x41"
        maxlength="4"
        @keyup.enter="handleSwapConfirm"
      />
    </div>
    <div class="flex justify-center">
      <ArrowLeftRight :size="18" class="text-gray-400 shrink-0" />
    </div>
    <div class="flex items-center gap-3">
      <label class="text-sm text-gray-600 select-none w-16">Char B:</label>
      <input
        v-model="hexB"
        class="flex-1 px-2 py-1 border border-gray-300 rounded text-sm font-mono select-none"
        placeholder="0x61"
        maxlength="4"
        @keyup.enter="handleSwapConfirm"
      />
    </div>
  </Dialog>

  <Dialog
    :open="dialogMode === 'duplicate'"
    title="Duplicate Characters"
    :error="dupError"
    wide
    @confirm="handleDuplicateConfirm"
    @cancel="closeDialog"
  >
    <p class="text-xs text-gray-500 select-none">Single: 0x41　Range: 0x01-0x03</p>
    <div class="flex items-center gap-3">
      <label class="text-sm text-gray-600 select-none w-16">Source:</label>
      <input
        ref="dupInputSrc"
        v-model="hexA"
        class="flex-1 px-2 py-1 border border-gray-300 rounded text-sm font-mono select-none"
        placeholder="0x41  or  0x01-0x03"
        @keyup.enter="handleDuplicateConfirm"
        @input="dupError = null"
      />
    </div>
    <div class="flex justify-center">
      <Copy :size="18" class="text-gray-400 shrink-0" />
    </div>
    <div class="flex items-center gap-3">
      <label class="text-sm text-gray-600 select-none w-16">Dest:</label>
      <input
        v-model="hexB"
        class="flex-1 px-2 py-1 border border-gray-300 rounded text-sm font-mono select-none"
        placeholder="0x61  or  0x04-0x06"
        @keyup.enter="handleDuplicateConfirm"
        @input="dupError = null"
      />
    </div>
  </Dialog>

  <Dialog
    :open="dialogMode === 'preview'"
    title="Font Preview"
    xwide
    close-only
    @cancel="closeDialog"
  >
    <FontPreview
      ref="previewRef"
      :data="fontBytes!"
      :bytes-per-char="bytesPerChar"
      :width="config?.font_width ?? 0"
      :height="config?.font_height ?? 0"
    />
  </Dialog>
</template>

<script setup lang="ts">
import { ref, nextTick } from 'vue'
import {
  FolderOpen,
  Download,
  Brush,
  Eraser,
  Trash2,
  ArrowLeftRight,
  Copy,
  Eye,
} from 'lucide-vue-next'
import type { FontConfig } from '../utils/types'
import Dialog from './Dialog.vue'
import FontPreview from './FontPreview.vue'

defineProps<{
  hasFont: boolean
  config: FontConfig | null
  tool: 'brush' | 'eraser'
  fontBytes: Uint8Array | null
  bytesPerChar: number
}>()

const emit = defineEmits<{
  'load-file': [content: string]
  'export': []
  'set-tool': [tool: 'brush' | 'eraser']
  'clear': []
  'swap': [a: number, b: number]
  'duplicate': [src: number, dest: number]
  'duplicate-range': [sources: number[], dests: number[]]
}>()

const fileInput = ref<HTMLInputElement>()
const swapInputA = ref<HTMLInputElement>()
const dupInputSrc = ref<HTMLInputElement>()
const previewRef = ref<InstanceType<typeof FontPreview>>()

const dialogMode = ref<'swap' | 'duplicate' | 'preview' | null>(null)
const hexA = ref('')
const hexB = ref('')
const dupError = ref<string | null>(null)

function parseHex(s: string): number | null {
  const v = parseInt(s, 16)
  return isNaN(v) || v < 0 || v > 255 ? null : v
}

function parseHexRange(raw: string): number[] | null {
  const s = raw.trim()
  if (!s) return null

  const rangeMatch = s.match(/^(0x[0-9a-fA-F]{1,2})\s*-\s*(0x[0-9a-fA-F]{1,2})$/)
  if (rangeMatch) {
    const start = parseHex(rangeMatch[1])
    const end = parseHex(rangeMatch[2])
    if (start === null || end === null) return null
    if (end < start) return null
    const result: number[] = []
    for (let i = start; i <= end; i++) {
      result.push(i)
    }
    return result
  }

  const singleMatch = s.match(/^0x[0-9a-fA-F]{1,2}$/)
  if (singleMatch) {
    const v = parseHex(s)
    return v !== null ? [v] : null
  }

  return null
}

function openDialog(mode: 'swap' | 'duplicate') {
  dialogMode.value = mode
  hexA.value = ''
  hexB.value = ''
  dupError.value = null
  nextTick(() => {
    if (mode === 'swap') {
      swapInputA.value?.focus()
    } else {
      dupInputSrc.value?.focus()
    }
  })
}

function closeDialog() {
  dialogMode.value = null
}

function openPreview() {
  dialogMode.value = 'preview'
  nextTick(() => {
    previewRef.value?.focus()
  })
}

function handleSwapConfirm() {
  const a = parseHex(hexA.value)
  const b = parseHex(hexB.value)
  if (a !== null && b !== null) {
    emit('swap', a, b)
  }
  closeDialog()
}

function handleDuplicateConfirm() {
  const sources = parseHexRange(hexA.value)
  const dests   = parseHexRange(hexB.value)

  if (!sources || sources.length === 0) {
    dupError.value = 'Invalid source: enter 0xNN or 0xNN-0xMM'
    return
  }
  if (!dests || dests.length === 0) {
    dupError.value = 'Invalid dest: enter 0xNN or 0xNN-0xMM'
    return
  }
  if (sources.length !== dests.length) {
    dupError.value = `Count mismatch: source has ${sources.length} value(s), dest has ${dests.length} value(s). They must be equal.`
    return
  }

  for (const v of sources) {
    if (v < 0 || v > 255) {
      dupError.value = `Source value 0x${v.toString(16).toUpperCase()} out of 0–255 range`
      return
    }
  }
  for (const v of dests) {
    if (v < 0 || v > 255) {
      dupError.value = `Dest value 0x${v.toString(16).toUpperCase()} out of 0–255 range`
      return
    }
  }

  if (sources.length === 1) {
    emit('duplicate', sources[0], dests[0])
  } else {
    emit('duplicate-range', sources, dests)
  }
  closeDialog()
}

function openFile() {
  fileInput.value?.click()
}

function handleFileSelect(event: Event) {
  const input = event.target as HTMLInputElement
  const file = input.files?.[0]
  if (!file) return
  const reader = new FileReader()
  reader.onload = () => {
    emit('load-file', reader.result as string)
  }
  reader.readAsText(file)
  input.value = ''
}

function handleExport() {
  emit('export')
}
</script>
