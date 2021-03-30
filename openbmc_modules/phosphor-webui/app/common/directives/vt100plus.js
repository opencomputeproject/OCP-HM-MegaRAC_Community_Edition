'use strict';

var EscapeSequences = require('xterm/lib/common/data/EscapeSequences');

var BACKSPACE = 8;
var PAGE_UP = 33;
var PAGE_DOWN = 34;
var END = 35;
var HOME = 36;
var INSERT = 45;
var DEL = 46;
var F1 = 112;
var F2 = 113;
var F3 = 114;
var F4 = 115;
var F5 = 116;
var F6 = 117;
var F7 = 118;
var F8 = 119;
var F9 = 120;
var F10 = 121;
var F11 = 122;
var F12 = 123;

/*
VT100+ Character and Key Extensions

Character or key  | Character sequence
---------------------------------------
HOME key          | <ESC>h
END key           | <ESC>k
INSERT key        | <ESC>+
DELETE key        | <ESC>-
PAGE UP key       | <ESC>?
PAGE DOWN key     | <ESC>/
F1 key            | <ESC>1
F2 key            | <ESC>2
F3 key            | <ESC>3
F4 key            | <ESC>4
F5 key            | <ESC>5
F6 key            | <ESC>6
F7 key            | <ESC>7
F8 key            | <ESC>8
F9 key            | <ESC>9
F10 key           | <ESC>0
F11 key           | <ESC>!
F12 key           | <ESC>@

*/

function customVT100PlusKey(ev, term) {
  var modifiers = (ev.shiftKey ? 1 : 0) | (ev.altKey ? 2 : 0) |
      (ev.ctrlKey ? 4 : 0) | (ev.metaKey ? 8 : 0);
  if (((modifiers) && (ev.keyCode != BACKSPACE)) || (ev.type != 'keydown')) {
    return true;
  }
  switch (ev.keyCode) {
    case BACKSPACE:
      if (ev.altKey) {
        return true;
      } else if (!ev.shiftKey) {
        term.handler(EscapeSequences.C0.BS);  // Backspace
      } else {
        term.handler(EscapeSequences.C0.DEL);  // Delete
      }
      break;
    case PAGE_UP:
      term.handler(EscapeSequences.C0.ESC + '?');
      break;
    case PAGE_DOWN:
      term.handler(EscapeSequences.C0.ESC + '/');
      break;
    case END:
      term.handler(EscapeSequences.C0.ESC + 'k');
      break;
    case HOME:
      term.handler(EscapeSequences.C0.ESC + 'h');
      break;
    case INSERT:
      term.handler(EscapeSequences.C0.ESC + '+');
      break;
    case DEL:
      term.handler(EscapeSequences.C0.ESC + '-');
      break;
    case F1:
      term.handler(EscapeSequences.C0.ESC + '1');
      break;
    case F2:
      term.handler(EscapeSequences.C0.ESC + '2');
      break;
    case F3:
      term.handler(EscapeSequences.C0.ESC + '3');
      break;
    case F4:
      term.handler(EscapeSequences.C0.ESC + '4');
      break;
    case F5:
      term.handler(EscapeSequences.C0.ESC + '5');
      break;
    case F6:
      term.handler(EscapeSequences.C0.ESC + '6');
      break;
    case F7:
      term.handler(EscapeSequences.C0.ESC + '7');
      break;
    case F8:
      term.handler(EscapeSequences.C0.ESC + '8');
      break;
    case F9:
      term.handler(EscapeSequences.C0.ESC + '9');
      break;
    case F10:
      term.handler(EscapeSequences.C0.ESC + '0');
      break;
    case F11:
      term.handler(EscapeSequences.C0.ESC + '!');
      break;
    case F12:
      term.handler(EscapeSequences.C0.ESC + '@');
      break;
    default:
      return true;
  }
  return false;
}
exports.customVT100PlusKey = customVT100PlusKey;
