

### Derivatives and Partial Derivatives

**Derivatives:** Measures the rate of change of a function at a specific point. It dictates the direction in which a function scales up or down.

**Partial Derivatives:** When a Machine Learning model handles millions of weights, we compute the derivative with respect to one variable while holding all other variables constant.

**The Gradient Vector:** A vector grouping all partial derivatives together. It points directly toward the direction of the steepest ascent.

### Chain Rule and Loss Functions

**Chain Rule:** Used to calculate the derivative of nested composite functions. This is the absolute foundation of the Backpropagation algorithm in Deep Learning.

**Loss Function / Cost Function:** A function measuring the error between model predictions and reality (e.g., MSE). Our ultimate goal is to minimize this function.

### Gradient Descent Optimization

**Intuition:** Imagine standing on a foggy mountaintop wanting to reach the valley (the minimum loss). You take steps in the opposite direction of the Gradient (the steepest descent).

**Learning Rate - α:** The step size. If it is too large, you overshoot the valley; if it is too small, convergence takes forever.
