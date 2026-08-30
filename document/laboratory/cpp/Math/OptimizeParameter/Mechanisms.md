
### Gradient Descent Optimization

**Intuition:** Imagine standing on a foggy mountaintop wanting to reach the valley (the minimum loss). You take steps in the opposite direction of the Gradient (the steepest descent).

**Learning Rate - α:** The step size. If it is too large, you overshoot the valley; if it is too small, convergence takes forever.

### The Complete Gradient Descent Optimization Algorithm.

Parameter Update Formula:

\(x_{new}=x_{old}-\alpha \cdot \nabla f(x_{old})\)

Where:
\(\nabla f(x)\) The Gradient vector (slope direction).
\(\alpha \) (Alpha is Learning Rate): The Learning Rate: the size of each step.

In Machine Learning, we typically have:

\(X\): (Input data). Input feature matrix.
\(W\): (Weights matrix) Weights matrix that the model needs to learn.
\(Y_{pred} = X \cdot W\): Predicted output matrix.

The Loss function evaluates the error of \(Y_{pred}\). Consequently, the Gradient Vector turns into a Gradient Matrix (\(\nabla _{W}L\)) matching the exact dimensions of \(W\). Each element in this gradient matrix is the partial derivative of the loss with respect to that specific weight:

\(W_{new}=W_{old}-\alpha \cdot \nabla _{W}L\)
