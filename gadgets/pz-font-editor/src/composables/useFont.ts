import { ref, computed } from 'vue'
import type { FontData, FontConfig } from '../utils/types'
import { parseHeaderFile, getCharBitmap, setCharBitmap, getCharWidth, setCharWidth } from '../utils/fontParser'
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
    return getCharBitmap(data, bytesPerChar, selectedCharIndex.value, cfg.font_width, cfg.font_height, cfg.var_width)
  })

  const charWidths = computed(() => fontData.value?.charWidths ?? null)
  const currentCharWidth = computed(() => {
    if (!fontData.value) return 0
    const { data, bytesPerChar, config: cfg } = fontData.value
    if (!cfg.var_width) return cfg.font_width
    return getCharWidth(data, bytesPerChar, selectedCharIndex.value)
  })

  function createNewFont(config: FontConfig): boolean {
    config.var_width = config.var_width ?? false
    const bytesPerChar = Math.ceil(config.font_width / 8) * config.font_height + (config.var_width ? 1 : 0)
    const data = new Uint8Array(bytesPerChar * 256)
    let charWidths: Uint8Array | undefined
    if (config.var_width) {
      charWidths = new Uint8Array(256)
      for (let i = 0; i < 256; i++) {
        data[i * bytesPerChar] = config.font_width
        charWidths[i] = config.font_width
      }
    }
    fontData.value = { config, data, bytesPerChar, charCount: 256, charWidths }
    selectedCharIndex.value = 0
    version.value++
    return true
  }

  function updateSettings(c_variable: string, var_width: boolean): boolean {
    if (!fontData.value) return false
    const oldConfig = fontData.value.config
    if (oldConfig.var_width === var_width && oldConfig.c_variable === c_variable) return true

    if (oldConfig.var_width !== var_width) {
      const oldBpc = Math.ceil(oldConfig.font_width / 8) * oldConfig.font_height + (oldConfig.var_width ? 1 : 0)
      const newBpc = Math.ceil(oldConfig.font_width / 8) * oldConfig.font_height + (var_width ? 1 : 0)
      const newData = new Uint8Array(newBpc * 256)
      const bitmapBytes = Math.ceil(oldConfig.font_width / 8) * oldConfig.font_height
      let newCharWidths: Uint8Array | undefined

      for (let i = 0; i < 256; i++) {
        const oldBitmapStart = i * oldBpc + (oldConfig.var_width ? 1 : 0)
        const newStart = i * newBpc
        if (var_width) {
          newData[newStart] = oldConfig.var_width
            ? fontData.value.data[i * oldBpc]
            : oldConfig.font_width
          newData.set(fontData.value.data.slice(oldBitmapStart, oldBitmapStart + bitmapBytes), newStart + 1)
        } else {
          newData.set(fontData.value.data.slice(oldBitmapStart, oldBitmapStart + bitmapBytes), newStart)
        }
      }

      if (var_width) {
        newCharWidths = new Uint8Array(256)
        for (let i = 0; i < 256; i++) {
          newCharWidths[i] = newData[i * newBpc]
        }
      }

      fontData.value = {
        config: { ...oldConfig, c_variable, var_width },
        data: newData,
        bytesPerChar: newBpc,
        charCount: 256,
        charWidths: newCharWidths,
      }
    } else {
      fontData.value = {
        ...fontData.value,
        config: { ...oldConfig, c_variable },
      }
    }
    version.value++
    return true
  }

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
    setCharBitmap(data, bytesPerChar, selectedCharIndex.value, cfg.font_width, cfg.font_height, newBitmap, cfg.var_width)
    version.value++
    fontData.value = { ...fontData.value }
  }

  function applyCharWidth(w: number): void {
    if (!fontData.value) return
    const { data, bytesPerChar, config: cfg } = fontData.value
    if (!cfg.var_width) return
    const idx = selectedCharIndex.value
    setCharWidth(data, bytesPerChar, idx, w)
    if (fontData.value.charWidths) {
      fontData.value.charWidths[idx] = w
    }
    version.value++
    fontData.value = { ...fontData.value }
  }

  function swapChars(a: number, b: number): boolean {
    if (!fontData.value || a === b) return false
    const { data, bytesPerChar, config: cfg } = fontData.value
    if (a < 0 || a >= 256 || b < 0 || b >= 256) return false

    const bitmapA = getCharBitmap(data, bytesPerChar, a, cfg.font_width, cfg.font_height, cfg.var_width)
    const bitmapB = getCharBitmap(data, bytesPerChar, b, cfg.font_width, cfg.font_height, cfg.var_width)

    setCharBitmap(data, bytesPerChar, a, cfg.font_width, cfg.font_height, bitmapB, cfg.var_width)
    setCharBitmap(data, bytesPerChar, b, cfg.font_width, cfg.font_height, bitmapA, cfg.var_width)

    if (cfg.var_width) {
      const wA = getCharWidth(data, bytesPerChar, a)
      const wB = getCharWidth(data, bytesPerChar, b)
      setCharWidth(data, bytesPerChar, a, wB)
      setCharWidth(data, bytesPerChar, b, wA)
      if (fontData.value.charWidths) {
        const cw = fontData.value.charWidths
        const tmp = cw[a]; cw[a] = cw[b]; cw[b] = tmp
      }
    }

    version.value++
    fontData.value = { ...fontData.value }
    return true
  }

  function duplicateChar(src: number, dest: number): boolean {
    if (!fontData.value || src === dest) return false
    const { data, bytesPerChar, config: cfg } = fontData.value
    if (src < 0 || src >= 256 || dest < 0 || dest >= 256) return false

    const bitmapSrc = getCharBitmap(data, bytesPerChar, src, cfg.font_width, cfg.font_height, cfg.var_width)
    setCharBitmap(data, bytesPerChar, dest, cfg.font_width, cfg.font_height, bitmapSrc, cfg.var_width)

    if (cfg.var_width) {
      const wSrc = getCharWidth(data, bytesPerChar, src)
      setCharWidth(data, bytesPerChar, dest, wSrc)
      if (fontData.value.charWidths) {
        fontData.value.charWidths[dest] = wSrc
      }
    }

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
      const bitmapSrc = getCharBitmap(data, bytesPerChar, src, cfg.font_width, cfg.font_height, cfg.var_width)
      setCharBitmap(data, bytesPerChar, dest, cfg.font_width, cfg.font_height, bitmapSrc, cfg.var_width)

      if (cfg.var_width) {
        const wSrc = getCharWidth(data, bytesPerChar, src)
        setCharWidth(data, bytesPerChar, dest, wSrc)
        if (fontData.value.charWidths) {
          fontData.value.charWidths[dest] = wSrc
        }
      }
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
    charWidths,
    currentCharWidth,
    createNewFont,
    updateSettings,
    loadFromContent,
    applyBitmap,
    applyCharWidth,
    swapChars,
    duplicateChar,
    duplicateRange,
    exportData,
    selectCharacter,
  }
}
