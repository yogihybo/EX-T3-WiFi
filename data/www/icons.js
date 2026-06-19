import { FA_ICONS } from './fa-icons.js';

export default {
  props: {
    active: Boolean
  },
  data() {
    return { icons: FA_ICONS };
  },
  template: `
  <div>
    <p class="text-muted small mb-3">Icons available on the device display. Select one when configuring function buttons.</p>
    <div class="fa-icon-grid">
      <div v-for="item of icons" :key="item.icon" class="fa-icon-cell" :title="item.icon">
        <i :class="['fa-solid', 'fa-' + item.icon]"></i>
        <span>{{ item.label }}</span>
      </div>
    </div>
  </div>
  `
}
