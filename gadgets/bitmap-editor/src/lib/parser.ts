import type { Bitmap } from '../types'

function spriteDataToPixels(data: number[], width: number, height: number): boolean[][] {
  const cols = Math.ceil(width / 8)
  const pixels: boolean[][] = Array.from({ length: height }, () => Array(width).fill(false))

  for (let y = 0; y < height; y++) {
    const spriteRow = Math.floor(y / 8)
    const localY = y % 8
    for (let x = 0; x < width; x++) {
      const spriteCol = Math.floor(x / 8)
      const localX = x % 8
      const byteOffset = spriteRow * cols * 8 + spriteCol * 8 + localY
      const bitPos = 7 - localX
      if (byteOffset < data.length) {
        pixels[y][x] = ((data[byteOffset] >> bitPos) & 1) === 1
      }
    }
  }

  return pixels
}

export function parseHFile(content: string): Bitmap[] {
  const bitmaps: Bitmap[] = []
  const blockRegex = /\/\* #BITMAP_START \*\/([\s\S]*?)\/\* #BITMAP_END \*\//g
  let match

  while ((match = blockRegex.exec(content)) !== null) {
    const block = match[1]

    const jsonMatch = block.match(/\/\*\s*(\{[\s\S]*?\})\s*\*\//)
    if (!jsonMatch) continue

    let info: { variable_name: string; width: number; height: number }
    try {
      info = JSON.parse(jsonMatch[1])
    } catch {
      continue
    }

    const arrayMatch = block.match(
      /static\s+const\s+unsigned\s+char\s+\w+\[\]\s*=\s*\{([^}]+)\};/
    )
    if (!arrayMatch) continue

    const cleanContent = arrayMatch[1]
      .replace(/\/\*[\s\S]*?\*\//g, '')
      .replace(/\/\/.*$/gm, '')

    const values = cleanContent
      .split(',')
      .map(s => s.trim())
      .filter(s => s.length > 0)
      .map(s => {
        if (s.startsWith('0x') || s.startsWith('0X')) {
          return parseInt(s, 16)
        }
        return parseInt(s, 10)
      })
      .filter(n => !isNaN(n))

    if (values.length < 2) continue

    const width = values[0]
    const height = values[1]
    const spriteData = values.slice(2)

    const pixels = spriteDataToPixels(spriteData, width, height)
    bitmaps.push({ name: info.variable_name, width, height, pixels })
  }

  return bitmaps
}
