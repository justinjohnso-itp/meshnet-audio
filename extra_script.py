Import("env")

# Get the PIOENV (e.g., "s3-tx", "s3-rx", "c6-tx", "c6-rx")
pio_env = env["PIOENV"]

# All envs use the same main.c now (TX/RX determined by build flag UNIT_TX/UNIT_RX)
app_sources = "main.c"

if "tx" in pio_env:
    message = "Building TX firmware"
else:
    message = "Building RX firmware"

print(message)

# Generate src/CMakeLists.txt
cmake_content = f"""
idf_component_register(SRCS "{app_sources}")
"""

with open("src/CMakeLists.txt", "w") as f:
    f.write(cmake_content)
