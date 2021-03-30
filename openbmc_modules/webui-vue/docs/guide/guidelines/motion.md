# Motion
The motion guidelines are based on Carbon Design System guidelines. These guidelines avoid easing curves that are unnatural, distracting, or purely decorative. [The documentation below is attributed to Carbon's animation documentation](https://www.carbondesignsystem.com/guidelines/motion/basics/).


## Easing

### Productive motion
Productive motion creates a sense of efficiency and responsiveness, while remaining subtle and out of the way. Productive motion is appropriate for moments when the user needs to focus on completing tasks.

### Expressive motion
Expressive motion delivers enthusiastic, vibrant, and highly visible movement. Use expressive motion for significant moments such as opening a new page, clicking the primary action button, or when the movement itself conveys a meaning. System alerts and the appearance of notification boxes are great cases for expressive motion.

### Easing tokens
```
$standard-easing--productive: cubic-bezier(0.2, 0, 0.38, 0.9);
$standard-easing--expressive: cubic-bezier(0.4, 0.14, 0.3, 1);
$entrance-easing--productive: cubic-bezier(0, 0, 0.38, 0.9);
$entrance-easing--expressive: cubic-bezier(0, 0, 0.3, 1);
$exit-easing--productive: cubic-bezier(0.2, 0, 1, 0.9);
$exit-easing--expressive: cubic-bezier(0.4, 0.14, 1, 1);
```

## Duration
Duration is calculated based on the style and size of the motion. Among the two motion styles, productive motion is significantly faster than expressive motion. Motion’s duration should be dynamic based on the size of the animation; the larger the change in distance (traveled) or size (scaling) of the element, the longer the animation takes.

### Duration tokens
```
$duration--fast-01: 70ms; //Micro-interactions such as button and toggle
$duration--fast-02: 110ms; //Micro-interactions such as fade
$duration--moderate-01: 150ms; //Micro-interactions, small expansion, short distance movements
$duration--moderate-02: 240ms; //Expansion, system communication, toast
$duration--slow-01: 400ms; //Large expansion, important system notifications
$duration--slow-02: 700ms; //Background dimming
```