<script setup lang="ts">
import { ref } from 'vue'
import { Check, Expand, Pencil } from '@lucide/vue'
import MenuBar from '../components/MenuBar.vue'
import EditorCanvas from '../components/EditorCanvas.vue'
import LayerList from '../components/LayerList.vue'
import {
  cIdentifierRegex,
  bitmaps,
  selectedIndex,
  editingPixels,
  selectedBitmap,
  tool,
  canvasVersion,
  loadedFileName,
  errorMessage,
  selectBitmap,
  applyEdits,
  togglePixel,
  addBitmap,
  resizeBitmap,
  renameBitmap,
  importBMPPixels,
  loadFromContent,
  exportContent,
  deriveHeaderGuard,
  showError,
} from '../store/editor'
import { parseBMP } from '../lib/bmp'

const showNewDialog = ref(false)
const newName = ref('')
const newWidth = ref(8)
const newHeight = ref(8)
const newNameError = ref('')
const fileInput = ref<HTMLInputElement | null>(null)
const bmpInput = ref<HTMLInputElement | null>(null)

const showSaveDialog = ref(false)
const saveFileName = ref('')
const saveFileNameError = ref('')

const showResizeDialog = ref(false)
const resizeWidth = ref(8)
const resizeHeight = ref(8)

const showRenameDialog = ref(false)
const renameName = ref('')
const renameNameError = ref('')

function handleLoad() {
  fileInput.value?.click()
}

function handleFileChange(e: Event) {
  const input = e.target as HTMLInputElement
  const file = input.files?.[0]
  if (!file) return
  const reader = new FileReader()
  reader.onload = () => {
    loadFromContent(reader.result as string, file.name)
  }
  reader.readAsText(file)
  input.value = ''
}

function handleSave() {
  if (loadedFileName.value) {
    const guard = deriveHeaderGuard(loadedFileName.value)
    doExport(guard, loadedFileName.value)
  } else {
    saveFileName.value = ''
    saveFileNameError.value = ''
    showSaveDialog.value = true
  }
}

function confirmSave() {
  const name = saveFileName.value.trim()
  if (!name) {
    saveFileNameError.value = 'File name is required'
    return
  }
  const baseName = name.replace(/\.[^.]+$/, '')
  if (!cIdentifierRegex.test(baseName)) {
    saveFileNameError.value =
      'File name must be a valid C identifier (start with letter/underscore, letters/digits/underscores only)'
    return
  }
  const fileName = name.endsWith('.h') ? name : name + '.h'
  const guard = deriveHeaderGuard(fileName)
  doExport(guard, fileName)
  showSaveDialog.value = false
}

function doExport(guard: string, fileName: string) {
  const content = exportContent(guard, fileName)
  const blob = new Blob([content], { type: 'text/plain' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = fileName
  a.click()
  URL.revokeObjectURL(url)
}

function handleNew() {
  newName.value = ''
  newWidth.value = 8
  newHeight.value = 8
  newNameError.value = ''
  showNewDialog.value = true
}

function confirmNew() {
  const name = newName.value.trim()
  if (!name) return
  if (!cIdentifierRegex.test(name)) {
    newNameError.value =
      'Name must be a valid C identifier (start with letter/underscore, letters/digits/underscores only)'
    return
  }
  if (newWidth.value < 1 || newWidth.value > 255) return
  if (newHeight.value < 1 || newHeight.value > 255) return
  addBitmap(name, newWidth.value, newHeight.value)
  showNewDialog.value = false
}

function handleNewNameInput() {
  newNameError.value = ''
}

function handleSaveNameInput() {
  saveFileNameError.value = ''
}

function handleResize() {
  if (!editingPixels.value) return
  resizeWidth.value = editingPixels.value[0].length
  resizeHeight.value = editingPixels.value.length
  showResizeDialog.value = true
}

function confirmResize() {
  if (resizeWidth.value < 1 || resizeWidth.value > 255) return
  if (resizeHeight.value < 1 || resizeHeight.value > 255) return
  resizeBitmap(resizeWidth.value, resizeHeight.value)
  showResizeDialog.value = false
}

function handleRename() {
  if (!selectedBitmap.value) return
  renameName.value = selectedBitmap.value.name
  renameNameError.value = ''
  showRenameDialog.value = true
}

function confirmRename() {
  const name = renameName.value.trim()
  if (!name) return
  if (!cIdentifierRegex.test(name)) {
    renameNameError.value =
      'Name must be a valid C identifier (start with letter/underscore, letters/digits/underscores only)'
    return
  }
  renameBitmap(name)
  showRenameDialog.value = false
}

function handleRenameNameInput() {
  renameNameError.value = ''
}

function handleImportBMP() {
  bmpInput.value?.click()
}

function handleBMPChange(e: Event) {
  const input = e.target as HTMLInputElement
  const file = input.files?.[0]
  if (!file) return
  const reader = new FileReader()
  reader.onload = () => {
    try {
      const result = parseBMP(reader.result as ArrayBuffer)
      importBMPPixels(result.pixels)
    } catch (err) {
      showError(err instanceof Error ? err.message : 'Failed to parse BMP file')
    }
  }
  reader.onerror = () => {
    showError('Failed to read BMP file')
  }
  reader.readAsArrayBuffer(file)
  input.value = ''
}
</script>

<template>
  <div class="flex flex-col h-screen bg-white text-zinc-900">
    <input ref="fileInput" type="file" accept=".h" class="hidden" @change="handleFileChange" />
    <input ref="bmpInput" type="file" accept=".bmp" class="hidden" @change="handleBMPChange" />

    <MenuBar
      :editing="editingPixels !== null"
      :tool="tool"
      @load="handleLoad"
      @save="handleSave"
      @new-bitmap="handleNew"
      @import-b-m-p="handleImportBMP"
      @update:tool="tool = $event"
    />

    <main class="flex flex-1 min-h-0">
      <section class="flex-1 flex flex-col min-w-0 border-r border-zinc-200">
        <div
          v-if="!editingPixels"
          class="flex-1 flex items-center justify-center text-zinc-400 text-sm"
        >
          Select a bitmap from the list to edit
        </div>
        <template v-else>
          <div class="flex-1 overflow-auto p-4 flex items-center justify-center bg-zinc-50">
            <EditorCanvas
              :pixels="editingPixels"
              :canvas-version="canvasVersion"
              @toggle-pixel="togglePixel"
            />
          </div>
          <div
            class="flex items-center justify-between px-4 py-3 border-t border-zinc-200 bg-white shrink-0"
          >
            <div class="flex items-center gap-2">
              <button
                class="inline-flex items-center gap-1.5 px-3 py-1.5 text-sm rounded bg-white text-zinc-700 border border-zinc-300 hover:bg-violet-50 hover:border-violet-300 transition-colors cursor-pointer"
                @click="handleResize"
              >
                <Expand class="w-3.5 h-3.5" />
                Resize
              </button>
              <button
                class="inline-flex items-center gap-1.5 px-3 py-1.5 text-sm rounded bg-white text-zinc-700 border border-zinc-300 hover:bg-violet-50 hover:border-violet-300 transition-colors cursor-pointer"
                @click="handleRename"
              >
                <Pencil class="w-3.5 h-3.5" />
                Rename
              </button>
            </div>
            <button
              class="inline-flex items-center gap-1.5 px-4 py-1.5 text-sm rounded bg-violet-600 text-white font-medium hover:bg-violet-700 transition-colors cursor-pointer"
              @click="applyEdits"
            >
              <Check class="w-3.5 h-3.5" />
              Apply
            </button>
          </div>
        </template>
      </section>

      <LayerList
        :bitmaps="bitmaps"
        :selected-index="selectedIndex"
        @select="selectBitmap"
      />
    </main>

    <Transition
      enter-active-class="transition-opacity duration-200"
      leave-active-class="transition-opacity duration-300"
      enter-from-class="opacity-0"
      leave-to-class="opacity-0"
    >
      <div
        v-if="errorMessage"
        class="fixed top-16 left-1/2 -translate-x-1/2 z-50 px-4 py-2 text-sm rounded bg-red-600 text-white shadow-lg"
      >
        {{ errorMessage }}
      </div>
    </Transition>

    <Teleport to="body">
      <div
        v-if="showNewDialog"
        class="fixed inset-0 flex items-center justify-center bg-black/30 z-50"
        @mousedown.self="showNewDialog = false"
      >
        <div class="bg-white border border-zinc-200 rounded-lg p-6 w-80 shadow-lg">
          <h2 class="text-sm font-semibold text-zinc-900 mb-4">New Bitmap</h2>
          <div class="space-y-3">
            <div>
              <label class="block text-xs text-zinc-500 mb-1">Name</label>
              <input
                v-model="newName"
                class="w-full px-3 py-1.5 text-sm rounded border outline-none"
                :class="
                  newNameError
                    ? 'border-red-400 focus:border-red-500'
                    : 'border-zinc-200 focus:border-violet-400'
                "
                placeholder="BITMAP_NAME"
                @keyup.enter="confirmNew"
                @input="handleNewNameInput"
              />
              <p v-if="newNameError" class="text-xs text-red-500 mt-1">{{ newNameError }}</p>
            </div>
            <div class="flex gap-3">
              <div class="flex-1">
                <label class="block text-xs text-zinc-500 mb-1">Width</label>
                <input
                  v-model.number="newWidth"
                  type="number"
                  min="1"
                  max="255"
                  class="w-full px-3 py-1.5 text-sm rounded border border-zinc-200 outline-none focus:border-violet-400"
                />
              </div>
              <div class="flex-1">
                <label class="block text-xs text-zinc-500 mb-1">Height</label>
                <input
                  v-model.number="newHeight"
                  type="number"
                  min="1"
                  max="255"
                  class="w-full px-3 py-1.5 text-sm rounded border border-zinc-200 outline-none focus:border-violet-400"
                />
              </div>
            </div>
          </div>
          <div class="flex justify-end gap-2 mt-5">
            <button
              class="px-3 py-1.5 text-sm rounded bg-white text-zinc-600 border border-zinc-300 hover:bg-zinc-50 transition-colors cursor-pointer"
              @click="showNewDialog = false"
            >
              Cancel
            </button>
            <button
              class="px-3 py-1.5 text-sm rounded bg-violet-600 text-white font-medium hover:bg-violet-700 transition-colors cursor-pointer"
              @click="confirmNew"
            >
              Create
            </button>
          </div>
        </div>
      </div>

      <div
        v-if="showSaveDialog"
        class="fixed inset-0 flex items-center justify-center bg-black/30 z-50"
        @mousedown.self="showSaveDialog = false"
      >
        <div class="bg-white border border-zinc-200 rounded-lg p-6 w-80 shadow-lg">
          <h2 class="text-sm font-semibold text-zinc-900 mb-4">Save As</h2>
          <div>
            <label class="block text-xs text-zinc-500 mb-1">File name</label>
            <input
              v-model="saveFileName"
              class="w-full px-3 py-1.5 text-sm rounded border outline-none"
              :class="
                saveFileNameError
                  ? 'border-red-400 focus:border-red-500'
                  : 'border-zinc-200 focus:border-violet-400'
              "
              placeholder="bitmaps.h"
              @keyup.enter="confirmSave"
              @input="handleSaveNameInput"
            />
            <p v-if="saveFileNameError" class="text-xs text-red-500 mt-1">{{ saveFileNameError }}</p>
          </div>
          <div class="flex justify-end gap-2 mt-5">
            <button
              class="px-3 py-1.5 text-sm rounded bg-white text-zinc-600 border border-zinc-300 hover:bg-zinc-50 transition-colors cursor-pointer"
              @click="showSaveDialog = false"
            >
              Cancel
            </button>
            <button
              class="px-3 py-1.5 text-sm rounded bg-violet-600 text-white font-medium hover:bg-violet-700 transition-colors cursor-pointer"
              @click="confirmSave"
            >
              Save
            </button>
          </div>
        </div>
      </div>

      <div
        v-if="showResizeDialog"
        class="fixed inset-0 flex items-center justify-center bg-black/30 z-50"
        @mousedown.self="showResizeDialog = false"
      >
        <div class="bg-white border border-zinc-200 rounded-lg p-6 w-72 shadow-lg">
          <h2 class="text-sm font-semibold text-zinc-900 mb-4">Resize Bitmap</h2>
          <div class="flex gap-3">
            <div class="flex-1">
              <label class="block text-xs text-zinc-500 mb-1">Width</label>
              <input
                v-model.number="resizeWidth"
                type="number"
                min="1"
                max="255"
                class="w-full px-3 py-1.5 text-sm rounded border border-zinc-200 outline-none focus:border-violet-400"
              />
            </div>
            <div class="flex-1">
              <label class="block text-xs text-zinc-500 mb-1">Height</label>
              <input
                v-model.number="resizeHeight"
                type="number"
                min="1"
                max="255"
                class="w-full px-3 py-1.5 text-sm rounded border border-zinc-200 outline-none focus:border-violet-400"
              />
            </div>
          </div>
          <div class="flex justify-end gap-2 mt-5">
            <button
              class="px-3 py-1.5 text-sm rounded bg-white text-zinc-600 border border-zinc-300 hover:bg-zinc-50 transition-colors cursor-pointer"
              @click="showResizeDialog = false"
            >
              Cancel
            </button>
            <button
              class="px-3 py-1.5 text-sm rounded bg-violet-600 text-white font-medium hover:bg-violet-700 transition-colors cursor-pointer"
              @click="confirmResize"
            >
              Resize
            </button>
          </div>
        </div>
      </div>

      <div
        v-if="showRenameDialog"
        class="fixed inset-0 flex items-center justify-center bg-black/30 z-50"
        @mousedown.self="showRenameDialog = false"
      >
        <div class="bg-white border border-zinc-200 rounded-lg p-6 w-80 shadow-lg">
          <h2 class="text-sm font-semibold text-zinc-900 mb-4">Rename Bitmap</h2>
          <div>
            <label class="block text-xs text-zinc-500 mb-1">Name</label>
            <input
              v-model="renameName"
              class="w-full px-3 py-1.5 text-sm rounded border outline-none"
              :class="
                renameNameError
                  ? 'border-red-400 focus:border-red-500'
                  : 'border-zinc-200 focus:border-violet-400'
              "
              placeholder="BITMAP_NAME"
              @keyup.enter="confirmRename"
              @input="handleRenameNameInput"
            />
            <p v-if="renameNameError" class="text-xs text-red-500 mt-1">{{ renameNameError }}</p>
          </div>
          <div class="flex justify-end gap-2 mt-5">
            <button
              class="px-3 py-1.5 text-sm rounded bg-white text-zinc-600 border border-zinc-300 hover:bg-zinc-50 transition-colors cursor-pointer"
              @click="showRenameDialog = false"
            >
              Cancel
            </button>
            <button
              class="px-3 py-1.5 text-sm rounded bg-violet-600 text-white font-medium hover:bg-violet-700 transition-colors cursor-pointer"
              @click="confirmRename"
            >
              Rename
            </button>
          </div>
        </div>
      </div>
    </Teleport>
  </div>
</template>
