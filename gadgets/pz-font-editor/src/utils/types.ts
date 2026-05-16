export interface FontConfig {
  c_variable: string
  font_name: string
  font_width: number
  font_height: number
}

export interface FontData {
  config: FontConfig
  data: Uint8Array
  bytesPerChar: number
  charCount: number
}
