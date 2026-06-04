import type { FontData } from './types'
import { getCharName } from './fontMapping'

export function exportHeaderFile(fontData: FontData): string {
  const { config, data, bytesPerChar, charWidths } = fontData

  const commentJson = JSON.stringify(config, null, 4)
    .split('\n')
    .map((line, i) => i === 0 ? line : '    ' + line)
    .join('\n')

  let maxCommentWidth = 0
  for (let i = 0; i < 256; i++) {
    const comment = `/* ${getCharName(i)} */`
    if (comment.length > maxCommentWidth) {
      maxCommentWidth = comment.length
    }
  }
  const indent = '    '

  const lines: string[] = []
  lines.push('/*')
  lines.push(commentJson)
  lines.push('*/')
  lines.push(`static const unsigned char ${config.c_variable}[] = {`)

  for (let i = 0; i < 256; i++) {
    const name = getCharName(i)
    const comment = `/* ${name} */`
    const padding = ' '.repeat(maxCommentWidth - comment.length + 2)
    const offset = i * bytesPerChar
    const bitmapStart = config.var_width ? offset + 1 : offset
    const bitmapBytes = config.var_width ? bytesPerChar - 1 : bytesPerChar
    const chunk = data.slice(bitmapStart, bitmapStart + bitmapBytes)

    let hexChunk: string
    if (config.var_width && charWidths) {
      const w = '0x' + charWidths[i].toString(16).padStart(2, '0').toUpperCase()
      const bitmapHex = Array.from(chunk, (b) => '0x' + b.toString(16).padStart(2, '0').toUpperCase()).join(', ')
      hexChunk = w + '/* Width */, ' + bitmapHex
    } else {
      hexChunk = Array.from(chunk, (b) => '0x' + b.toString(16).padStart(2, '0').toUpperCase()).join(', ')
    }
    const comma = i < 255 ? ',' : ''
    lines.push(indent + comment + padding + hexChunk + comma)
  }

  lines.push('};')
  return lines.join('\n') + '\n'
}
