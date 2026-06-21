// Built-in default function sets — read-only, cannot be edited or deleted.
// Users can copy these to create a customisable version.

const IDLE    = (icon) => ({ label: null, icon, color: 0xC618, fill: 0x2104, border: 0x4208 });
const PRESSED = (icon, color = 0x07FF, border = 0x07FF) => ({ label: null, icon, color, fill: 0x2104, border });

// colour themes for pressed state
const YELLOW = 0xFFE0;  // lights
const ORANGE = 0xFC00;  // sound / horn
const RED    = 0xF800;  // brake / safety
const CYAN   = 0x07FF;  // mechanical / general

const fn = (fnNum, name, latching, idleIcon, pressedIcon, pressedColor = CYAN) => ({
  fn: fnNum,
  name,
  latching,
  idleDisplay:    'icon',
  pressedDisplay: 'icon',
  btn: {
    idle:    IDLE(idleIcon),
    pressed: PRESSED(pressedIcon, pressedColor, pressedColor),
  }
});

// FA codepoints
const CP = {
  lightbulb:             0xf0eb,
  'lightbulb-on':        0xf672,
  bell:                  0xf0f3,
  'bell-slash':          0xf1f6,
  whistle:               0xf460,
  'light-emergency':     0xe41f,
  'light-emergency-on':  0xe420,
  siren:                 0xe02d,
  'siren-on':            0xe02e,
  link:                  0xf0c1,
  'link-slash':          0xf127,
  'volume-high':         0xf028,
  'volume-xmark':        0xf6a9,
  bullseye:              0xf140,
  'brake-warning':       0xe0c7,
  fan:                   0xf863,
  gears:                 0xf085,
  'wind-turbine':        0xf89b,
  smoke:                 0xf760,
  fire:                  0xf06d,
  'oil-can':             0xf613,
  'oil-can-drip':        0xe205,
};

const BASIC_FUNCTIONS = [
  fn(0,  'Headlights',    true,  CP.lightbulb,          CP['lightbulb-on'],       YELLOW),
  fn(1,  'Bell',          true,  CP.bell,               CP.bell,                  ORANGE),
  fn(2,  'Horn',          false, CP.whistle,            CP.whistle,               ORANGE),
  fn(3,  'Dynamic Brake', true,  CP.bullseye,           CP.bullseye,              RED),
  fn(4,  'Ditch Lights',  true,  CP['light-emergency'], CP['light-emergency-on'], YELLOW),
  fn(5,  'Cab Light',     true,  CP.lightbulb,          CP['lightbulb-on'],       YELLOW),
  fn(6,  'Strobe',        true,  CP.siren,              CP['siren-on'],           YELLOW),
  fn(7,  'Coupler',       false, CP.link,               CP['link-slash'],         CYAN),
  fn(8,  'Mute',          true,  CP['volume-high'],     CP['volume-xmark'],       RED),
];

const EXTENDED_FUNCTIONS = [
  ...BASIC_FUNCTIONS,
  fn(9,  'Cooling Fan',    true,  CP.fan,              CP.fan,              CYAN),
  fn(10, 'Prime Mover',    true,  CP.gears,            CP.gears,            CYAN),
  fn(11, 'Air Compressor', true,  CP['wind-turbine'],  CP['wind-turbine'],  CYAN),
  fn(12, 'Brake Squeal',   false, CP['brake-warning'], CP['brake-warning'], RED),
  fn(13, 'Smoke Unit',     true,  CP.smoke,            CP.smoke,            CYAN),
  fn(14, 'Firebox',        true,  CP.fire,             CP.fire,             ORANGE),
  fn(15, 'Injector',       true,  CP['oil-can'],       CP['oil-can-drip'],  CYAN),
];

export const FN_DEFAULTS = [
  {
    name:      'Default - Basic (F0-F8)',
    functions: BASIC_FUNCTIONS.map(f => [f]),
  },
  {
    name:      'Default - Extended (F0-F15)',
    functions: EXTENDED_FUNCTIONS.map(f => [f]),
  },
];
