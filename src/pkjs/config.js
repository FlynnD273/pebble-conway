/**
 * "EdgeWrap",
 * "FPS",
 * "CellSize",
 * "BGColor",
 * "FGColor"
*/
module.exports = [
  {
    "type": "heading",
    "defaultValue": "App Configuration"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Game Settings"
      },
      {
        "type": "toggle",
        "messageKey": "EdgeWrap",
        "label": "Wrap around edges",
        "defaultValue": true
      },
      {
        "type": "slider",
        "messageKey": "CellSize",
        "label": "Cell size",
        "defaultValue": 5,
        "min": 3,
        "max": 20
      },
      {
        "type": "slider",
        "messageKey": "FPS",
        "label": "FPS",
        "defaultValue": 12,
        "min": 1,
        "max": 60
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Colors"
      },
      {
        "type": "color",
        "messageKey": "FGColor",
        "label": "Foreground color",
        "defaultValue": "0x000000"
      },
      {
        "type": "color",
        "messageKey": "BGColor",
        "label": "Background color",
        "defaultValue": "0xFFFFFF"
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
