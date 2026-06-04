export interface FontConfig {
  c_variable: string
  font_name: string
  font_width: number
  font_height: number
  var_width: boolean
}

export interface FontData {
  config: FontConfig
  data: Uint8Array
  bytesPerChar: number
  charCount: number
  charWidths?: Uint8Array
}
