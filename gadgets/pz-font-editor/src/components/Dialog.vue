<template>
  <Teleport to="body">
    <div
      v-if="open"
      class="fixed inset-0 z-50 flex items-center justify-center"
    >
      <div class="absolute inset-0 bg-black/30" @click="handleCancel" />
      <div
        class="relative bg-white rounded-lg shadow-xl p-6 flex flex-col gap-3"
        :class="xwide ? 'w-[40rem]' : wide ? 'w-[30rem]' : 'w-[22rem]'"
      >
        <slot name="title">
          <h3 class="text-base font-semibold text-gray-800 select-none">{{ title }}</h3>
        </slot>
        <slot />
        <div
          v-if="error"
          class="bg-red-50 border border-red-200 text-red-700 text-sm rounded px-3 py-2 select-none"
        >
          {{ error }}
        </div>
        <div v-if="closeOnly" class="flex justify-end">
          <button
            class="px-4 py-1.5 border border-gray-300 rounded text-sm hover:bg-gray-50 cursor-pointer select-none"
            @click="handleCancel"
          >
            Close
          </button>
        </div>
        <div v-else class="flex justify-end gap-2">
          <button
            class="px-4 py-1.5 border border-gray-300 rounded text-sm hover:bg-gray-50 cursor-pointer select-none"
            @click="handleCancel"
          >
            Cancel
          </button>
          <button
            class="px-4 py-1.5 bg-blue-500 text-white rounded text-sm hover:bg-blue-600 cursor-pointer select-none"
            @click="handleConfirm"
          >
            OK
          </button>
        </div>
      </div>
    </div>
  </Teleport>
</template>

<script setup lang="ts">
defineProps<{
  open: boolean
  title: string
  error?: string | null
  wide?: boolean
  xwide?: boolean
  closeOnly?: boolean
}>()

const emit = defineEmits<{
  'confirm': []
  'cancel': []
}>()

function handleConfirm() {
  emit('confirm')
}

function handleCancel() {
  emit('cancel')
}
</script>
