<script setup lang="ts">
import { FolderOpen, Download, Plus, Image, Pen, Eraser } from '@lucide/vue'

defineProps<{
  editing: boolean
  tool: 'pen' | 'eraser'
}>()

const emit = defineEmits<{
  load: []
  save: []
  newBitmap: []
  importBMP: []
  'update:tool': [value: 'pen' | 'eraser']
}>()

function setTool(t: 'pen' | 'eraser') {
  emit('update:tool', t)
}
</script>

<template>
  <header class="flex items-center justify-between px-4 h-12 border-b border-zinc-200 shrink-0 bg-white">
    <div class="flex items-center gap-3">
      <span class="font-semibold text-sm text-zinc-900 tracking-wide">Bitmap Editor</span>
      <template v-if="editing">
        <div class="w-px h-5 bg-zinc-200" />
        <button
          :class="[
            'inline-flex items-center gap-1.5 px-3 py-1 text-sm rounded border transition-colors cursor-pointer',
            tool === 'pen'
              ? 'bg-violet-600 text-white border-violet-600'
              : 'bg-white text-zinc-700 border-zinc-300 hover:bg-violet-50 hover:border-violet-300',
          ]"
          @click="setTool('pen')"
        >
          <Pen class="w-3.5 h-3.5" />
          Pen
        </button>
        <button
          :class="[
            'inline-flex items-center gap-1.5 px-3 py-1 text-sm rounded border transition-colors cursor-pointer',
            tool === 'eraser'
              ? 'bg-violet-600 text-white border-violet-600'
              : 'bg-white text-zinc-700 border-zinc-300 hover:bg-violet-50 hover:border-violet-300',
          ]"
          @click="setTool('eraser')"
        >
          <Eraser class="w-3.5 h-3.5" />
          Eraser
        </button>
      </template>
    </div>

    <nav class="flex gap-2">
      <button
        v-if="editing"
        class="inline-flex items-center gap-1.5 px-3 py-1 text-sm rounded bg-white text-zinc-700 border border-zinc-300 hover:bg-violet-50 hover:border-violet-300 transition-colors cursor-pointer"
        @click="$emit('importBMP')"
      >
        <Image class="w-3.5 h-3.5" />
        Import
      </button>
      <div class="w-px h-5 bg-zinc-200 self-center" />
      <button
        class="inline-flex items-center gap-1.5 px-3 py-1 text-sm rounded bg-white text-zinc-700 border border-zinc-300 hover:bg-violet-50 hover:border-violet-300 transition-colors cursor-pointer"
        @click="$emit('load')"
      >
        <FolderOpen class="w-3.5 h-3.5" />
        Load
      </button>
      <button
        class="inline-flex items-center gap-1.5 px-3 py-1 text-sm rounded bg-violet-600 text-white border border-violet-600 hover:bg-violet-700 transition-colors cursor-pointer"
        @click="$emit('save')"
      >
        <Download class="w-3.5 h-3.5" />
        Save
      </button>
      <button
        class="inline-flex items-center gap-1.5 px-3 py-1 text-sm rounded bg-white text-zinc-700 border border-zinc-300 hover:bg-violet-50 hover:border-violet-300 transition-colors cursor-pointer"
        @click="$emit('newBitmap')"
      >
        <Plus class="w-3.5 h-3.5" />
        New
      </button>
    </nav>
  </header>
</template>
