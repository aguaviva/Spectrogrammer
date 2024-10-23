import math
sample_rate = 48000.0
min_freq = (sample_rate/2.0)/4096.0
max_freq = 4096.0*min_freq
max_x = 1.0

a = min_freq
b = math.log10(max_freq / a)/max_x
inva = 1.0 / a
invb = 1.0 / b

def forward(x):
    return a * pow(10, b * x)

def backward(x):
    return math.log10(x * inva) * invb;

print(min_freq, max_freq)

print(backward(min_freq))
print(backward(max_freq))

print(forward(0))
print(forward(1))

print(forward(backward(min_freq)))
print(forward(backward(max_freq)))
print(backward(forward(0)))
print(backward(forward(1)))
