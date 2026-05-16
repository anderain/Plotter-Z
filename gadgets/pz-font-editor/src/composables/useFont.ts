import { ref, computed } from 'vue'
import type { FontData } from '../utils/types'
import { parseHeaderFile, getCharBitmap, setCharBitmap } from '../utils/fontParser'
import { exportHeaderFile } from '../utils/fontExporter'

export function useFont() {
  const fontData = ref<FontData | null>(null)
  const selectedCharIndex = ref(0)
  const version = ref(0)

  const config = computed(() => fontData.value?.config ?? null)
  const bitmap = computed(() => {
    void version.value
    if (!fontData.value) return null
    const { data, bytesPerChar, config: cfg } = fontData.value
    return getCharBitmap(data, bytesPerChar, selectedCharIndex.value, cfg.font_width, cfg.font_height)
  })

  function loadFromContent(content: string): boolean {
    const parsed = parseHeaderFile(content)
    if (!parsed) return false
    fontData.value = parsed
    selectedCharIndex.value = 0
    version.value++
    return true
  }

  function applyBitmap(newBitmap: boolean[][]): void {
    if (!fontData.value) return
    const { data, bytesPerChar, config: cfg } = fontData.value
    setCharBitmap(data, bytesPerChar, selectedCharIndex.value, cfg.font_width, cfg.font_height, newBitmap)
    version.value++
    fontData.value = { ...fontData.value }
  }

  function swapChars(a: number, b: number): boolean {
    if (!fontData.value || a === b) return false
    const { data, bytesPerChar, config: cfg } = fontData.value
    if (a < 0 || a >= 256 || b < 0 || b >= 256) return false

    const bitmapA = getCharBitmap(data, bytesPerChar, a, cfg.font_width, cfg.font_height)
    const bitmapB = getCharBitmap(data, bytesPerChar, b, cfg.font_width, cfg.font_height)

    setCharBitmap(data, bytesPerChar, a, cfg.font_width, cfg.font_height, bitmapB)
    setCharBitmap(data, bytesPerChar, b, cfg.font_width, cfg.font_height, bitmapA)

    version.value++
    fontData.value = { ...fontData.value }
    return true
  }

  function duplicateChar(src: number, dest: number): boolean {
    if (!fontData.value || src === dest) return false
    const { data, bytesPerChar, config: cfg } = fontData.value
    if (src < 0 || src >= 256 || dest < 0 || dest >= 256) return false

    const bitmapSrc = getCharBitmap(data, bytesPerChar, src, cfg.font_width, cfg.font_height)
    setCharBitmap(data, bytesPerChar, dest, cfg.font_width, cfg.font_height, bitmapSrc)

    version.value++
    fontData.value = { ...fontData.value }
    return true
  }

  function duplicateRange(sources: number[], dests: number[]): boolean {
    if (!fontData.value || sources.length !== dests.length) return false
    const { data, bytesPerChar, config: cfg } = fontData.value

    for (let i = 0; i < sources.length; i++) {
      const src = sources[i]
      const dest = dests[i]
      if (src < 0 || src >= 256 || dest < 0 || dest >= 256) return false
      if (src === dest) continue
      const bitmapSrc = getCharBitmap(data, bytesPerChar, src, cfg.font_width, cfg.font_height)
      setCharBitmap(data, bytesPerChar, dest, cfg.font_width, cfg.font_height, bitmapSrc)
    }

    version.value++
    fontData.value = { ...fontData.value }
    return true
  }

  function exportData(): string | null {
    if (!fontData.value) return null
    return exportHeaderFile(fontData.value)
  }

  function selectCharacter(index: number): void {
    selectedCharIndex.value = index
  }

  return {
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
  }
}
