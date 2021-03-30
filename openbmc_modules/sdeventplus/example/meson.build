examples = [
  'delayed_echo',
  'follow',
  'heartbeat',
  'heartbeat_timer',
]

foreach example : examples
  executable(example, example + '.cpp',
             implicit_include_directories: false,
             dependencies: sdeventplus)
endforeach
