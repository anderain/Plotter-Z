import type { FontConfig, FontData } from './types'

function computeBytesPerChar(width: number, height: number): number {
  return Math.ceil(width / 8) * height
}

export function parseHeaderFile(content: string): FontData | null {
  const commentMatch = content.match(/\/\*\s*([\s\S]*?)\s*\*\//)
  if (!commentMatch) return null

  const commentBody = commentMatch[1].trim()
  let config: FontConfig
  try {
    config = JSON.parse(commentBody) as FontConfig
  } catch {
    return null
  }

  if (!config.font_width || !config.font_height || !config.c_variable) {
    return null
  }

  const bytesPerChar = computeBytesPerChar(config.font_width, config.font_height)

  const hexValues: number[] = []
  const hexRegex = /0x[0-9a-fA-F]{2}/g
  const arraySection = content.slice(commentMatch.index! + commentMatch[0].length)
  let match: RegExpExecArray | null
  while ((match = hexRegex.exec(arraySection)) !== null) {
    hexValues.push(parseInt(match[0], 16))
  }

  const totalBytes = bytesPerChar * 256
  if (hexValues.length < totalBytes) {
    return null
  }

  const data = new Uint8Array(hexValues.slice(0, totalBytes))
  return { config, data, bytesPerChar, charCount: 256 }
}

export function getCharBitmap(data: Uint8Array, bytesPerChar: number, charIndex: number, width: number, height: number): boolean[][] {
  const offset = charIndex * bytesPerChar
  const bytesPerRow = Math.ceil(width / 8)
  const bitmap: boolean[][] = []

  for (let row = 0; row < height; row++) {
    const rowPixels: boolean[] = []
    for (let col = 0; col < width; col++) {
      const byteIndex = offset + row * bytesPerRow + Math.floor(col / 8)
      const bitIndex = 7 - (col % 8)
      const bit = (data[byteIndex] >> bitIndex) & 1
      rowPixels.push(bit === 1)
    }
    bitmap.push(rowPixels)
  }
  return bitmap
}

export function setCharBitmap(data: Uint8Array, bytesPerChar: number, charIndex: number, width: number, height: number, bitmap: boolean[][]): void {
  const offset = charIndex * bytesPerChar
  const bytesPerRow = Math.ceil(width / 8)

  for (let row = 0; row < height; row++) {
    for (let col = 0; col < width; col++) {
      const byteIndex = offset + row * bytesPerRow + Math.floor(col / 8)
      const bitIndex = 7 - (col % 8)
      if (bitmap[row][col]) {
        data[byteIndex] |= (1 << bitIndex)
      } else {
        data[byteIndex] &= ~(1 << bitIndex)
      }
    }
  }
}
