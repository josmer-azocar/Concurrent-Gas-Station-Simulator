# ⛽ Concurrent Gas Station Simulator

![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-blue?style=for-the-badge)
![Language](https://img.shields.io/badge/Language-C-00599C?style=for-the-badge&logo=c&logoColor=white)
![Concurrency](https://img.shields.io/badge/Concurrency-Pthreads-000000?style=for-the-badge&logo=linux&logoColor=white)

A multithreaded simulation in C that models and analyzes vehicle flow at a gas station with multiple pumps and a central waiting queue. This project demonstrates advanced handling of concurrency, thread synchronization, and shared resource management to create a robust and efficient system.

---

### 🎬 Demo

<img width="975" height="480" alt="imagen" src="https://github.com/user-attachments/assets/6b4c9d3f-348e-4b00-8de1-e983ec9aa528" />
<img width="645" height="304" alt="imagen" src="https://github.com/user-attachments/assets/0421719a-fc16-42be-b763-465f0eb84254" />
<img width="479" height="296" alt="imagen" src="https://github.com/user-attachments/assets/ab5415af-1db7-43e5-98a7-4ce1a1d4f5b7" />
<img width="462" height="302" alt="imagen" src="https://github.com/user-attachments/assets/d3b97686-1629-4e74-96c6-61a243d3cd9a" />

---

### ✨ Key Features

*   **Robust Concurrency Model:** Orchestrates 9 threads to simultaneously handle vehicle generation, the logic for each gas pump, the waiting queue, and the user interface.
*   **Thread-Safe Synchronization:** Utilizes `mutexes` to protect access to shared variables (queues, statistics), ensuring data integrity and preventing race conditions.
*   **Load Balancing Logic:** Arriving vehicles are automatically directed to the shortest pump queue to optimize overall wait times.
*   **Dynamic Console UI:** Real-time visualization of the simulation's state, clearly displaying queues, pumps, and the timer.
*   **Data Persistence:** Upon completion, the simulation automatically generates a `Simulation_Data.txt` file with a summary of performance metrics (vehicles served, average service times, etc.).
*   **Cross-Platform Compatibility:** Designed to compile and run seamlessly on both Windows and Linux/macOS environments.

---

### 🔧 Architecture and How It Works

The system operates on a **Producer-Consumer model**:

1.  **Generator Thread (Producer):** One thread is dedicated exclusively to creating "work" (vehicles) at random intervals.
2.  **Pump Threads (Consumers):** Four threads, one for each pump, "consume" the work by servicing the vehicles in their respective queues.
3.  **Management Threads:** Additional threads handle auxiliary tasks such as:
    *   Moving vehicles from the central waiting queue to the pumps.
    *   Updating the user interface in the console.
    *   Controlling the total simulation time.
    *   Generating the final statistics report.

All shared state (queues, time, stats) is protected by a **single mutex** to ensure that only one thread can modify the data at any given time.

---

### 🛠️ Technologies Used

*   **Language:** C
*   **Libraries:** `pthread` (for thread management), `stdlib`, `stdio`, `time`
*   **Compiler:** GCC / MinGW
*   **Platforms:** Windows, Linux, macOS

---

### 🚀 Getting Started

Ensure you have a C compiler (like GCC) installed on your system.

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/josmer-azocar/Concurrent-Gas-Station-Simulator.git
    ```
2.  **Navigate to the project directory:**
    ```bash
    cd Concurrent-Gas-Station-Simulator
    ```
3.  **Compile the program:**
    *The `-lpthread` flag is essential for linking the pthreads library.*
    ```bash
    gcc main.c -o simulator -lpthread
    ```
4.  **Run the simulation:**
    ```bash
    ./simulator
    ```
5.  Follow the on-screen prompts to configure the simulation parameters.
