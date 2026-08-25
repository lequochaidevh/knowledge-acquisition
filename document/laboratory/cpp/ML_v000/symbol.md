### 1. Notation Notes

Throughout my writings, mathematical variables and structures are represented using the following conventions (matching the standard syntax used in MATLAB):

* **Scalars**: Represented by non-bold letters, which can be either lowercase or uppercase (e.g., $x_1$, $N$, $y$, $k$).
* **Vectors**: Represented by lowercase bold letters (e.g., $\mathbf{y}$, $\mathbf{x}_1$). By default, all vectors are assumed to be **column vectors** unless specified otherwise.
  * **Row Vector**: Written with commas, for example: $\mathbf{x} = [x_1, x_2, \dots, x_n]$.
  * **Column Vector**: Written with semicolons, for example: $\mathbf{x} = [x_1; x_2; \dots; x_n]$.
* **Matrices**: Represented by uppercase bold letters (e.g., $\mathbf{X}$, $\mathbf{Y}$, $\mathbf{W}$).

#### Matrix Construction
* **Horizontal Concatenation**: $\mathbf{X} = [\mathbf{x}_1, \mathbf{x}_2, \dots, \mathbf{x}_n]$ means the column vectors $\mathbf{x}_j$ are placed side-by-side from left to right to form matrix $\mathbf{X}$.
* **Vertical Concatenation**: $\mathbf{X} = [\mathbf{x}_1; \mathbf{x}_2; \dots; \mathbf{x}_m]$ means the row vectors $\mathbf{x}_i$ are stacked on top of each other from top to bottom to form matrix $\mathbf{X}$.

*Note: All vectors are implicitly assumed to have matching dimensions appropriate for concatenation.*

#### Default Elements
For any given matrix $\mathbf{W}$, unless stated otherwise, $\mathbf{w}_i$ denotes the $i$-th **column vector** of that matrix. Please note the direct correspondence between the uppercase letter for the matrix and the lowercase letter for its constituent vectors.

---

To determine the distance between two vectors $\mathbf{y}$ and $\mathbf{z}$, a function is typically applied to the difference vector $\mathbf{x} = \mathbf{y} - \mathbf{z}$. A function used to measure vectors must satisfy several specific mathematical properties.

### Definition of a Norm
A function $f(\cdot)$ that maps a point $\mathbf{x}$ from an $n$-dimensional space to the one-dimensional set of real numbers is called a **norm** if it satisfies the following three conditions:

1. **Non-negativity**: $f(\mathbf{x}) \ge 0$. The equality holds $\iff \mathbf{x} = \mathbf{0}$.
2. **Absolute Homogeneity**: $f(\alpha \mathbf{x}) = |\alpha| f(\mathbf{x}), \quad \forall \alpha \in \mathbb{R}$.
3. **Triangle Inequality**: $f(\mathbf{x}_1) + f(\mathbf{x}_2) \ge f(\mathbf{x}_1 + \mathbf{x}_2), \quad \forall \mathbf{x}_1, \mathbf{x}_2 \in \mathbb{R}^n$.

#### Interpretation of the Conditions
* **The first condition** is intuitive because distance cannot be negative. Furthermore, the distance between two points $\mathbf{y}$ and $\mathbf{z}$ is zero if and only if the two points coincide, meaning $\mathbf{x} = \mathbf{y} - \mathbf{z} = \mathbf{0}$.
* **The second condition** can be explained as follows: If three points $\mathbf{y}$, $\mathbf{v}$, and $\mathbf{z}$ are collinear such that $\mathbf{v} - \mathbf{y} = \alpha(\mathbf{v} - \mathbf{z})$, then the distance between $\mathbf{v}$ and $\mathbf{y}$ will be $|\alpha|$ times the distance between $\mathbf{v}$ and $\mathbf{z}$.
* **The third condition** represents the classic triangle inequality if we consider $\mathbf{x}_1 = \mathbf{w} - \mathbf{y}$ and $\mathbf{x}_2 = \mathbf{z} - \mathbf{w}$, where $\mathbf{w}$ is any arbitrary point in the same space.

---

### Commonly Used Vector Norms
Assume the vectors are given as $\mathbf{x} = [x_1; x_2; \dots; x_n]$ and $\mathbf{y} = [y_1; y_2; \dots; y_n]$.

#### 1. The $\ell_2$ Norm (Euclidean Distance)
The standard Euclidean distance is a valid norm, commonly referred to as the **$\ell_2$ norm**:
$$\|\mathbf{x}\|_2 = \sqrt{x_1^2 + x_2^2 + \dots + x_n^2} \tag{1}$$

#### 2. The $\ell_p$ Norm
For any real number $p \ge 1$, the following function satisfies the three conditions above and is called the **$\ell_p$ norm**:
$$\|\mathbf{x}\|_p = \left(|x_1|^p + |x_2|^p + \dots + |x_n|^p\right)^{\frac{1}{p}} \tag{2}$$

#### 3. The $\ell_0$ Pseudo-Norm
As $p \to 0$, the expression above converges to the number of non-zero elements in $\mathbf{x}$. Equation (2) at $p = 0$ is called the **$\ell_0$ pseudo-norm**. It is not a true norm because it violates conditions 2 and 3. This pseudo-norm, denoted as $\|\mathbf{x}\|_0$, is highly important in Machine Learning because many problems require a "sparse" constraint—meaning the number of active (non-zero) components in $\mathbf{x}$ should be small.

#### Specific Values of $p$ Commonly Used:
* **When $p = 2$**: We obtain the $\ell_2$ norm as shown above.
* **When $p = 1$**: We obtain the **$\ell_1$ norm**:
  $$\|\mathbf{x}\|_1 = |x_1| + |x_2| + \dots + |x_n| \tag{3}$$
  This is the sum of the absolute values of each element in $\mathbf{x}$. The $\ell_1$ norm is frequently used as a convex approximation of the $\ell_0$ norm in optimization problems with sparsity constraints. 
  
  *Example (2D Space)*: The $\ell_2$ norm (green) represents the straight "as-the-crow-flies" line connecting vectors $\mathbf{x}$ and $\mathbf{y}$. The $\ell_1$ distance between these two points (red) can be interpreted as the path from $\mathbf{x}$ to $\mathbf{y}$ in a city grid (Manhattan distance). You can only travel along the edges of the grid and cannot cut across diagonally.
  
* **When $p \to \infty$**: We obtain the **$\ell_\infty$ norm** (Max norm), which equals the maximum absolute value among the elements of the vector:
  $$\|\mathbf{x}\|_\infty = \max_{i=1, 2, \dots, n} |x_i| \tag{4}$$

---

### Matrix Norms
For a matrix $\mathbf{A} \in \mathbb{R}^{m \times n}$, the most commonly used norm is the **Frobenius norm**, denoted as $\|\mathbf{A}\|_F$. It is defined as the square root of the sum of the absolute squares of all its elements:
$$\|\mathbf{A}\|_F = \sqrt{\sum_{i=1}^{m} \sum_{j=1}^{n} a_{ij}^2} \tag{5}$$