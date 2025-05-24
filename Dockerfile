# Use official gcc image with g++ pre-installed
FROM gcc:12

# Set working directory inside the container
WORKDIR /app

# Copy your source code and includes into the container
COPY . .

# Build your project
# Assuming your source files are in src/ and headers in include/
RUN g++ -std=c++17 -Iinclude main.cpp src/disk_manager.cpp src/record_manager.cpp -o dbms

# Default command to run your DBMS executable
CMD ["./dbms"]
