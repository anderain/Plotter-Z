export function parseBMP(buffer: ArrayBuffer): { width: number; height: number; pixels: boolean[][] } {
  const bytes = new Uint8Array(buffer)

  if (bytes[0] !== 0x42 || bytes[1] !== 0x4d) {
    throw new Error('Not a valid BMP file (missing BM signature)')
  }

  const dv = new DataView(buffer)
  const pixelOffset = dv.getUint32(10, true)
  const dibSize = dv.getUint32(14, true)

  if (dibSize < 40) {
    throw new Error('Unsupported BMP DIB header')
  }

  const width = dv.getInt32(18, true)
  const rawHeight = dv.getInt32(22, true)
  const topDown = rawHeight < 0
  const height = Math.abs(rawHeight)
  const bpp = dv.getUint16(28, true)
  const compression = dv.getUint32(30, true)

  if (compression !== 0) {
    throw new Error('Only uncompressed BMP files are supported')
  }
  if (bpp !== 24 && bpp !== 32) {
    throw new Error(`Unsupported BMP bit depth: ${bpp}. Only 24-bit and 32-bit are supported.`)
  }

  const bytesPerPixel = bpp / 8
  const rowSize = Math.floor((bpp * width + 31) / 32) * 4

  const pixels: boolean[][] = Array.from({ length: height }, () => Array(width).fill(false))

  for (let y = 0; y < height; y++) {
    const srcRow = topDown ? y : height - 1 - y
    const rowOffset = pixelOffset + srcRow * rowSize

    for (let x = 0; x < width; x++) {
      const pxOffset = rowOffset + x * bytesPerPixel
      const b = bytes[pxOffset]
      const g = bytes[pxOffset + 1]
      const r = bytes[pxOffset + 2]
      const brightness = (r + g + b) / 3
      pixels[y][x] = brightness < 128
    }
  }

  return { width, height, pixels }
}
