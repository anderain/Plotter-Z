<script setup lang="ts">
import { ref } from 'vue'
import { useFont } from './composables/useFont'
import MenuBar from './components/MenuBar.vue'
import EditorCanvas from './components/EditorCanvas.vue'
import CharacterList from './components/CharacterList.vue'

const {
  fontData,
  config,
  bitmap,
  selectedCharIndex,
  loadFromContent,
  applyBitmap,
  swapChars,
  duplicateChar,
  duplicateRange,
  exportData,
  selectCharacter,
} = useFont()

const tool = ref<'brush' | 'eraser'>('brush')
const dataVersion = ref(0)
const editorRef = ref<InstanceType<typeof EditorCanvas>>()

function handleLoadFile(content: string) {
  const ok = loadFromContent(content)
  if (ok) {
    tool.value = 'brush'
    dataVersion.value++
  }
}

function handleExport() {
  const result = exportData()
  if (!result) return
  const blob = new Blob([result], { type: 'text/plain' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = (config.value?.font_name ?? 'font') + '.h'
  a.click()
  URL.revokeObjectURL(url)
}

function handleApply(newBitmap: boolean[][]) {
  applyBitmap(newBitmap)
  dataVersion.value++
}

function handleClear() {
  editorRef.value?.clearAll()
}

function handleSwap(a: number, b: number) {
  swapChars(a, b)
  dataVersion.value++
}

function handleDuplicate(src: number, dest: number) {
  duplicateChar(src, dest)
  dataVersion.value++
}

function handleDuplicateRange(sources: number[], dests: number[]) {
  duplicateRange(sources, dests)
  dataVersion.value++
}

</script>

<template>
  <div class="h-screen flex flex-col">
    <MenuBar
      :has-font="fontData !== null"
      :config="config"
      :tool="tool"
      :font-bytes="fontData?.data ?? null"
      :bytes-per-char="fontData?.bytesPerChar ?? 0"
      @load-file="handleLoadFile"
      @export="handleExport"
      @set-tool="tool = $event"
      @clear="handleClear"
      @swap="handleSwap"
      @duplicate="handleDuplicate"
      @duplicate-range="handleDuplicateRange"
    />
    <div class="flex-1 w-5xl mx-auto flex overflow-hidden">
      <div class="w-1/2 flex-1 overflow-auto flex items-start justify-center pt-4 border-r border-gray-200 bg-gray-50">
        <EditorCanvas
          ref="editorRef"
          :bitmap="bitmap"
          :width="config?.font_width ?? 0"
          :height="config?.font_height ?? 0"
          :selected-index="selectedCharIndex"
          :tool="tool"
          @apply="handleApply"
        />
      </div>
      <div class="w-1/2 flex-shrink-0 overflow-hidden">
        <CharacterList
          :data="fontData?.data ?? null"
          :bytes-per-char="fontData?.bytesPerChar ?? 0"
          :width="config?.font_width ?? 0"
          :height="config?.font_height ?? 0"
          :char-count="256"
          :selected-index="selectedCharIndex"
          :data-version="dataVersion"
          @select="selectCharacter"
        />
      </div>
    </div>
  </div>
</template>
