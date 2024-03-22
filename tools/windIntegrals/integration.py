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

def bivariate_polynomial_model(xy, a, b, c, d, e, f):
    x, y = xy
    return a*x**2 + b*y**2 + c*x*y + d*x + e*y + f

import numpy as np
import sympy as sp
from scipy.optimize import curve_fit

# Define the symbolic variable
x, f = sp.symbols('x f')

# Example complex function (replace this with your actual function)
example_function =  -2.85714285714286e-7*f**2*40*x**7 - 4.0e-5*f**2*40*x**5 - 0.00166666666666667*f**2*40*x**3 + 1.0*40*x

# Convert the sympy function to a numerical function for evaluation
# numerical_function = sp.lambdify(x, example_function, 'numpy')
numerical_function = sp.lambdify((x, f), example_function, 'numpy')

# Generate sample points (x values) and evaluate the function at these points
# x_values = np.linspace(0, 10, 1000)  # 100 points between 0 and 10
# y_values = numerical_function(x_values)

x_values, y_values = np.meshgrid(np.linspace(0, 10, 100), np.linspace(0, 10, 100))  # Create a meshgrid for x and y
z_values = numerical_function(x_values, y_values)  # Evaluate the function on the grid

x_values_flat = x_values.flatten()
y_values_flat = y_values.flatten()
z_values_flat = z_values.flatten()

# Define the quadratic model for fitting
def quadratic_model(x, a, b, c):
    return a * x**2 + b * x + c

# Perform the curve fitting to find the best coefficients for the quadratic model
# coefficients, _ = curve_fit(quadratic_model, x_values, y_values)


coefficients, _ = curve_fit(bivariate_polynomial_model, (x_values_flat, y_values_flat), z_values_flat)

# Extract the fitted coefficients
# a_fit, b_fit, c_fit = coefficients

# Display the fitted quadratic equation
# print(f"Fitted quadratic equation: f(x) = {a_fit:.4f}x^2 + {b_fit:.4f}x + {c_fit:.4f}")

print(f"Fitted bivariate polynomial equation coefficients: {coefficients}")
