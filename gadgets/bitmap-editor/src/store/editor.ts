import { ref, computed } from 'vue'
import type { Bitmap } from '../types'
import { parseHFile } from '../lib/parser'
import { exportHFile } from '../lib/exporter'

export const cIdentifierRegex = /^[a-zA-Z_][a-zA-Z0-9_]*$/

export const bitmaps = ref<Bitmap[]>([])
export const selectedIndex = ref<number | null>(null)
export const editingPixels = ref<boolean[][] | null>(null)
export const tool = ref<'pen' | 'fill'>('pen')
export const activeColor = ref(true)
export const canvasVersion = ref(0)
export const loadedFileName = ref<string | null>(null)
export const errorMessage = ref<string | null>(null)

export const selectedBitmap = computed(() =>
  selectedIndex.value !== null ? bitmaps.value[selectedIndex.value] : null
)

export function showError(msg: string) {
  errorMessage.value = msg
  setTimeout(() => {
    if (errorMessage.value === msg) errorMessage.value = null
  }, 4000)
}

export function selectBitmap(index: number) {
  if (selectedIndex.value === index && editingPixels.value !== null) return
  applyEdits()
  selectedIndex.value = index
  const bm = bitmaps.value[index]
  if (bm) {
    editingPixels.value = bm.pixels.map(row => [...row])
    canvasVersion.value++
  }
}

export function applyEdits() {
  if (selectedIndex.value !== null && editingPixels.value) {
    bitmaps.value[selectedIndex.value] = {
      ...bitmaps.value[selectedIndex.value],
      pixels: editingPixels.value.map(row => [...row]),
    }
    editingPixels.value = null
  }
}

export function togglePixel(x: number, y: number) {
  if (!editingPixels.value) return
  if (y < 0 || y >= editingPixels.value.length) return
  if (x < 0 || x >= editingPixels.value[y].length) return
  editingPixels.value[y][x] = activeColor.value
  canvasVersion.value++
}

export function floodFill(x: number, y: number) {
  if (!editingPixels.value) return
  const pixels = editingPixels.value
  const h = pixels.length
  if (h === 0) return
  const w = pixels[0].length
  if (x < 0 || x >= w || y < 0 || y >= h) return

  const target = pixels[y][x]
  const fill = activeColor.value
  if (target === fill) return

  const stack: number[] = [y * w + x]

  while (stack.length > 0) {
    const idx = stack.pop()!
    const px = idx % w
    const py = (idx / w) | 0
    if (pixels[py][px] !== target) continue
    pixels[py][px] = fill

    if (px + 1 < w && pixels[py][px + 1] === target) stack.push(py * w + (px + 1))
    if (px - 1 >= 0 && pixels[py][px - 1] === target) stack.push(py * w + (px - 1))
    if (py + 1 < h && pixels[py + 1][px] === target) stack.push((py + 1) * w + px)
    if (py - 1 >= 0 && pixels[py - 1][px] === target) stack.push((py - 1) * w + px)
  }

  canvasVersion.value++
}

export function addBitmap(name: string, width: number, height: number) {
  const pixels: boolean[][] = Array.from({ length: height }, () => Array(width).fill(false))
  bitmaps.value.push({ name, width, height, pixels })
}

export function resizeBitmap(width: number, height: number) {
  if (selectedIndex.value === null) return
  const bm = bitmaps.value[selectedIndex.value]
  const oldW = bm.width
  const oldH = bm.height

  const newPixels: boolean[][] = Array.from({ length: height }, (_, y) =>
    Array.from({ length: width }, (_, x) => {
      if (x < oldW && y < oldH) return bm.pixels[y][x]
      return false
    })
  )

  bitmaps.value[selectedIndex.value] = { ...bm, width, height, pixels: newPixels }
  editingPixels.value = newPixels.map(row => [...row])
  canvasVersion.value++
}

export function renameBitmap(name: string) {
  if (selectedIndex.value === null) return
  bitmaps.value[selectedIndex.value] = {
    ...bitmaps.value[selectedIndex.value],
    name,
  }
}

export function importBMPPixels(pixels: boolean[][]) {
  if (!editingPixels.value) return
  const bmpW = pixels[0].length
  const bmpH = pixels.length
  const editingW = editingPixels.value[0].length
  const editingH = editingPixels.value.length

  if (bmpW !== editingW || bmpH !== editingH) {
    showError(`BMP size (${bmpW}x${bmpH}) does not match canvas size (${editingW}x${editingH})`)
    return
  }

  editingPixels.value = pixels.map(row => [...row])
  canvasVersion.value++
}

export function loadFromContent(content: string, fileName?: string) {
  bitmaps.value = parseHFile(content)
  selectedIndex.value = null
  editingPixels.value = null
  canvasVersion.value++
  loadedFileName.value = fileName ?? null
}

export function exportContent(headerGuard: string, fileName: string): string {
  applyEdits()
  return exportHFile(bitmaps.value, headerGuard, fileName)
}

export function deriveHeaderGuard(filename: string): string {
  let name = filename.replace(/\.[^.]+$/, '')
  name = name.toUpperCase()
  name = name.replace(/[^A-Z0-9_]/g, '_')
  return `_${name}_`
}
