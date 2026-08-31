###  The Optimization Roadmap

**Convex:** Has exactly one global minimum, like a bowl-shaped valley. Gradient Descent is guaranteed to find the absolute best point.

**Non-Convex:** Contains multiple local minima and saddle points. This is the biggest challenge in Deep Learning as models easily get trapped in sub-optimal valleys.

**Advanced Loss Functions:** Regression.
 Besides MSE, we use MAE, which uses absolute error derivatives to be resilient against data outliers.

**Classification:** Cross-Entropy Loss.
 Measures the distance between probability distributions, forcing classification models to separate labels accurately.

 **Modern Optimizers:**  In production, pure Gradient Descent is rarely used. Instead, we deploy Momentum (adding inertia to roll past shallow bumps) or Adam (automatically adaptive learning rates for each weight).

 ### Why Momentum? The Physics Intuition

In vanilla Gradient Descent, the model only looks at the current local gradient to make a step. This causes two massive issues:

**1. Oscillations:** If the valley is sharp horizontally but shallow vertically, pure Gradient Descent will oscillate wildly back and forth between walls instead of descending down the floor.

**2. Local Minima:** When hitting a shallow dip (local minimum), the gradient becomes 0, freezing the model permanently while missing the true global minimum further ahead.

**The Momentum Solution:** Treat the optimizer like a heavy physical ball rolling down a hill. As it rolls, it accumulates Velocity. 
When cross-oscillating, opposing forces cancel out, driving the ball straight forward.
Local Minima, When facing a fake local minimum, its Inertia gives it enough speed to roll over the bump and continue downward.

#####  The Mathematical Equations

**Update Velocity:**

\(V_{new}=\beta \cdot V_{old}+(1-\beta )\cdot \nabla _{W}L\)

**Update Weights:**

\(W_{new}=W_{old}-\alpha \cdot V_{new}\)

(Where):
\(\alpha \) (Alpha): The Learning Rate.
\(\beta \) (Beta): The Momentum coefficient (typically 0.9), representing "friction". It retains 90% of old velocity and adds 10% of the new gradient force.