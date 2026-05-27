<script setup lang="ts">
import type { Bitmap } from '../types'
import MiniCanvas from './MiniCanvas.vue'

defineProps<{
  bitmaps: Bitmap[]
  selectedIndex: number | null
}>()

defineEmits<{
  select: [index: number]
}>()
</script>

<template>
  <aside class="w-72 shrink-0 flex flex-col bg-zinc-50 border-l border-zinc-200">
    <div class="flex items-center px-4 h-10 border-b border-zinc-200 text-xs text-zinc-500 font-medium">
      Bitmaps
    </div>
    <div class="flex-1 overflow-y-auto">
      <ul class="p-2 space-y-1">
        <li
          v-for="(bm, i) in bitmaps"
          :key="i"
          :class="[
            'flex items-center gap-3 px-3 py-2 rounded cursor-pointer transition-colors',
            selectedIndex === i
              ? 'bg-violet-100 text-violet-900'
              : 'text-zinc-600 hover:bg-zinc-100 hover:text-zinc-900',
          ]"
          @click="$emit('select', i)"
        >
          <MiniCanvas :bitmap="bm" />
          <div class="min-w-0">
            <div class="text-sm truncate">{{ bm.name }}</div>
            <div class="text-xs" :class="selectedIndex === i ? 'text-violet-400' : 'text-zinc-400'">
              {{ bm.width }} x {{ bm.height }}
            </div>
          </div>
        </li>
      </ul>
      <div
        v-if="bitmaps.length === 0"
        class="flex items-center justify-center h-full text-sm text-zinc-400 p-4 text-center"
      >
        No bitmaps yet.<br />Load a .h file or create a new one.
      </div>
    </div>
  </aside>
</template>
