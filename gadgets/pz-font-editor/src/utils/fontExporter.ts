import type { FontData } from './types'

export function exportHeaderFile(fontData: FontData): string {
  const { config, data } = fontData

  const commentJson = JSON.stringify(config, null, 4)
    .split('\n')
    .map((line, i) => i === 0 ? line : '    ' + line)
    .join('\n')

  const lines: string[] = []
  lines.push('/*')
  lines.push(commentJson)
  lines.push('*/')
  lines.push(`static const unsigned char ${config.c_variable}[] = {`)

  const bytesPerLine = 16
  for (let i = 0; i < data.length; i += bytesPerLine) {
    const chunk = data.slice(i, Math.min(i + bytesPerLine, data.length))
    const hexChunk = Array.from(chunk, (b) => '0x' + b.toString(16).padStart(2, '0').toUpperCase())
    const line = '    ' + hexChunk.join(', ')
    lines.push(i + bytesPerLine >= data.length ? line : line + ',')
  }

  lines.push('};')
  return lines.join('\n') + '\n'
}
