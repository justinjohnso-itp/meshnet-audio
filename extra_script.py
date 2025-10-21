Import("env")

# Get the PIOENV (e.g., "s3-tx", "s3-rx", "c6-tx", "c6-rx")
pio_env = env["PIOENV"]

# Determine which main.c to use based on TX/RX environment
if "tx" in pio_env:
    app_sources = "tx/main.c"
    message = "Building TX firmware"
else:
    app_sources = "rx/main.c"
    message = "Building RX firmware"

print(message)

# Generate src/CMakeLists.txt dynamically
cmake_content = f"""idf_component_register(
    SRCS 
        "{app_sources}"
        "../lib/audio/tx_pipeline.c"
        "../lib/audio/rx_pipeline.c"
        "../lib/audio/src/vs1053_adafruit_adapter.cpp"
        "../lib/control/src/display_ssd1306.c"
        "../lib/control/src/buttons.c"
        "../lib/network/src/mesh_net.c"
    INCLUDE_DIRS
        "../lib/audio/include"
        "../lib/audio"
        "../lib/audio/vs1053"
        "../lib/control/include"
        "../lib/network/include"
        "../lib/config/include"
)
"""

with open("src/CMakeLists.txt", "w") as f:
    f.write(cmake_content)
