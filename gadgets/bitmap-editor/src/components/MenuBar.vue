<script setup lang="ts">
import { FolderOpen, Download, Plus, Image, Pen, PaintBucket } from '@lucide/vue'

defineProps<{
  editing: boolean
  tool: 'pen' | 'fill'
  activeColor: boolean
}>()

const emit = defineEmits<{
  load: []
  save: []
  newBitmap: []
  importBMP: []
  'update:tool': [value: 'pen' | 'fill']
  'update:activeColor': [value: boolean]
}>()

function setTool(t: 'pen' | 'fill') {
  emit('update:tool', t)
}
</script>

<template>
  <header class="flex items-center justify-between px-4 h-12 border-b border-zinc-200 shrink-0 bg-white">
    <div class="flex items-center gap-3">
      <span class="font-semibold text-sm text-zinc-900 tracking-wide">Bitmap Editor</span>
      <template v-if="editing">
        <div class="w-px h-5 bg-zinc-200" />
        <div class="flex items-center gap-1.5">
          <button
            class="w-5 h-5 cursor-pointer rounded-sm bg-zinc-900 border-2 transition-all"
            :class="activeColor ? 'border-violet-500 scale-110' : 'border-zinc-300'"
            title="Black (B)"
            @click="$emit('update:activeColor', true)"
          />
          <button
            class="w-5 h-5 cursor-pointer rounded-sm bg-white border-2 transition-all"
            :class="activeColor ? 'border-zinc-300' : 'border-violet-500 scale-110'"
            title="White (B)"
            @click="$emit('update:activeColor', false)"
          />
        </div>
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
            tool === 'fill'
              ? 'bg-violet-600 text-white border-violet-600'
              : 'bg-white text-zinc-700 border-zinc-300 hover:bg-violet-50 hover:border-violet-300',
          ]"
          @click="setTool('fill')"
        >
          <PaintBucket class="w-3.5 h-3.5" />
          Fill
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
