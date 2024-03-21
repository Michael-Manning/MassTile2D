from sympy import *

# g, t = symbols('g t')
# # Function f(t)
# f_t = g * (t**2 * 0.006 + 0.1)

# # Compute the definite integral from 0 to x
# F_x = integrate(f_t, (t, 0, t))





# sin factor approximation s is seglength

# x, f, s = symbols('x f s')
# f_t = (0.002 * f * x**3 + 0.1 * f * x) * s
# F_x = integrate(f_t, (x, 0, x))

# cos factor aproximation

# x, f, s = symbols('x f s')
# f_t = (1 - (0.002 * f * x**3 + 0.1 * f * x)**2 / 2) * s   
# F_x = integrate(f_t, (x, 0, x))


# print(F_x)
# print()



import numpy as np
import sympy as sp
from scipy.optimize import curve_fit

# Define the symbolic variable
x = sp.symbols('x')

# Example complex function (replace this with your actual function)
example_function = sp.sin(x) + 0.1 * x**2

# Convert the sympy function to a numerical function for evaluation
numerical_function = sp.lambdify(x, example_function, 'numpy')

# Generate sample points (x values) and evaluate the function at these points
x_values = np.linspace(0, 10, 100)  # 100 points between 0 and 10
y_values = numerical_function(x_values)

# Define the quadratic model for fitting
def quadratic_model(x, a, b, c):
    return a * x**2 + b * x + c

# Perform the curve fitting to find the best coefficients for the quadratic model
coefficients, _ = curve_fit(quadratic_model, x_values, y_values)

# Extract the fitted coefficients
a_fit, b_fit, c_fit = coefficients

# Display the fitted quadratic equation
f"Fitted quadratic equation: f(x) = {a_fit:.4f}x^2 + {b_fit:.4f}x + {c_fit:.4f}"
