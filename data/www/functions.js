import { rand } from './utils.js';
import FnEditor from './fn.editor.js';
import { FN_DEFAULTS } from './fn-defaults.js';

const Modal = {
  components: { FnEditor },
  props: {
    fn: [Boolean, Object],
  },
  data() {
    return {
      name: '',
      editor: null,
    }
  },
  emits: ['close', 'update'],
  watch: {
    fn: {
      immediate: true,
      async handler() {
        if (this.fn.file) {
          // existing user set — load from device
          const response = await fetch(this.fn.file);
          if (response.ok) {
            ({ name: this.name, functions: this.editor } = await response.json());
          }
        } else if (this.fn.data) {
          // copy of a default — pre-populate, save as new file
          this.name   = this.fn.data.name.replace(/^Default\s*—\s*/i, '') + ' (copy)';
          this.editor = JSON.parse(JSON.stringify(this.fn.data.functions));
        }
      }
    }
  },
  methods: {
    close() {
      this.name   = '';
      this.editor = null;
      this.$emit('close');
    },
    async save() {
      const put = async file => {
        const response = await fetch(file, {
          method: 'PUT',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ name: this.name, functions: this.editor }),
        });
        if (response.ok) {
          this.$emit('update', { file, name: this.name });
          this.close();
        }
      };

      if (this.fn.file) {
        put(this.fn.file);
      } else {
        const file = `/fns/${rand().toString().substring(0, 8)}.json`;
        put(file);
      }
    },
  },
  template: `
  <Teleport to="body">
    <div class="modal d-block">
      <div class="modal-dialog modal-lg modal-dialog-centered modal-dialog-scrollable">
        <form @submit.prevent="save" class="modal-content">
          <div class="modal-header">
            <h5 class="modal-title">{{ fn.data ? 'Copy Default Function Set' : 'Function Editor' }}</h5>
            <button @click="close" type="button" class="btn-close"></button>
          </div>
          <div class="modal-body">
            <div class="row">
              <div class="col-12">
                <div class="form-floating">
                  <input v-model="name" type="text" class="form-control" required maxlength="20" placeholder="Function Set Name" />
                  <label>Function Set Name</label>
                </div>
              </div>
            </div>
            <div class="row">
              <div class="col text-center">
                <FnEditor v-if="editor !== null" v-model="editor" class="mt-3" />
              </div>
            </div>
          </div>
          <div class="modal-footer">
            <button @click="close" type="button" class="btn btn-secondary">Close &amp; Discard</button>
            <button type="submit" class="btn btn-primary">Save Changes</button>
          </div>
        </form>
      </div>
    </div>
    <div class="modal-backdrop fade show"></div>
  </Teleport>
  `
};

export default {
  components: { Modal },
  props: { active: Boolean },
  data() {
    return {
      save:      false,
      fns:       [],
      isLoading: false,
      defaults:  FN_DEFAULTS,
    }
  },
  watch: {
    active: {
      handler(value) { if (value) this.load(); },
      immediate: true
    }
  },
  methods: {
    async load() {
      this.isLoading = true;
      const response = await fetch('/fns');
      this.fns = await response.json();
      this.isLoading = false;
    },
    add()        { this.save = true; },
    edit(fn)     { this.save = fn; },
    copy(def)    { this.save = { data: def }; },
    async del(fn) {
      if (confirm('Are you sure you want to delete this function set?')) {
        const response = await fetch(fn.file, { method: 'DELETE' });
        if (response.ok) {
          this.fns = this.fns.filter(({ file }) => file !== fn.file);
        }
      }
    },
    update(fn) {
      const existing = this.fns.findIndex(({ file }) => file === fn.file);
      if (existing !== -1) {
        this.fns[existing].name = fn.name;
      } else {
        this.fns.push(fn);
      }
    },
  },
  template: `
  <div>
    <div class="row">
      <div class="col-12">
        <ul :class="{ loading: isLoading }" class="list-group list-group-flush">

          <!-- Column header -->
          <li class="list-group-item py-1 border-bottom">
            <div class="row small text-muted fw-semibold">
              <div class="col">Name</div>
              <div class="col-auto" style="min-width: 80px;"></div>
            </div>
          </li>

          <!-- Built-in defaults (read-only) -->
          <li v-for="def of defaults" :key="def.name" class="list-group-item">
            <div class="row align-items-center">
              <div class="col d-flex align-items-center gap-2">
                <svg width="13" height="13" fill="currentColor" class="text-muted flex-shrink-0"><use xlink:href="bs.icons.svg#lock-fill"/></svg>
                <span>{{ def.name }}</span>
                <span class="badge text-bg-secondary" style="font-size:0.65rem;">built-in</span>
              </div>
              <div class="col-auto d-flex flex-nowrap gap-2">
                <button @click="copy(def)" class="btn btn-link p-0 d-flex align-items-center" title="Copy and customise">
                  <svg width="16" height="16" fill="currentColor"><use xlink:href="bs.icons.svg#copy"/></svg>
                </button>
              </div>
            </div>
          </li>

          <!-- Divider between defaults and user sets -->
          <li v-if="fns.length" class="list-group-item py-1 border-bottom">
            <div class="row small text-muted fw-semibold">
              <div class="col">Custom Sets</div>
            </div>
          </li>

          <!-- User-created sets -->
          <li v-for="fn of fns" :key="fn.file" class="list-group-item">
            <div class="row align-items-center">
              <div class="col">{{ fn.name }}</div>
              <div class="col-auto d-flex flex-nowrap gap-2">
                <button @click="edit(fn)" class="btn btn-link p-0 d-flex align-items-center" title="Edit function set">
                  <svg width="16" height="16" fill="currentColor"><use xlink:href="bs.icons.svg#pencil"/></svg>
                </button>
                <a class="btn btn-link p-0 d-flex align-items-center" :href="fn.file" download title="Download function config">
                  <svg width="16" height="16" fill="currentColor"><use xlink:href="bs.icons.svg#download"/></svg>
                </a>
                <button @click="del(fn)" class="btn btn-link p-0 d-flex align-items-center text-danger" title="Delete function set">
                  <svg width="16" height="16" fill="currentColor"><use xlink:href="bs.icons.svg#trash"/></svg>
                </button>
              </div>
            </div>
          </li>

          <!-- Empty state (no user sets yet) -->
          <li v-if="!fns.length && !isLoading" class="list-group-item">
            <div class="empty-state">
              <svg width="40" height="40" fill="currentColor"><use xlink:href="bs.icons.svg#lightning-charge"/></svg>
              <p>No custom function sets yet — copy a default above to get started</p>
            </div>
          </li>

          <!-- Add button -->
          <li class="list-group-item border-0 pt-2 pb-0">
            <button @click="add" type="button" class="btn add-row-btn w-100 py-2">
              <svg width="16" height="16" fill="currentColor" class="me-2" style="vertical-align: text-bottom;"><use xlink:href="bs.icons.svg#plus-lg"/></svg> Add Function Set
            </button>
          </li>

        </ul>
      </div>
    </div>
    <Modal :fn="save" v-if="save" @close="save = false" @update="update" />
  </div>
  `
}
