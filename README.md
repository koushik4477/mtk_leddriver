

# Register-Level Platform LED Driver (Linux Kernel)

## Overview

This project implements a modern Linux platform driver that demonstrates:

* Memory-mapped register programming (MMIO)
* `regmap` abstraction
* Interrupt handling
* Clock framework integration
* Runtime power management
* Suspend / Resume callbacks
* Spinlock-based concurrency protection
* Basic performance measurement
* Device Tree binding

The driver simulates a hardware LED controller and exposes a sysfs interface for toggling the device.

This project models the architecture used in real embedded SoC peripheral drivers such as camera, audio, power, and display blocks.

---

## Architecture

User Space
→ sysfs interface
→ VFS
→ Platform Driver
→ regmap
→ MMIO registers
→ Hardware Block

The driver follows modern Linux kernel design patterns used in embedded systems.

---

## Features

### 1. Memory-Mapped Register Access

* `devm_ioremap_resource`
* `readl` / `writel` through regmap
* Register offset mapping
* Bit masking operations

### 2. Interrupt Handling

* `request_irq`
* Interrupt status clearing
* Spinlock protection in IRQ context

### 3. Clock Framework Integration

* `clk_prepare_enable`
* `clk_disable_unprepare`
* Simulated clock gating

### 4. Runtime Power Management

* `pm_runtime_enable`
* `pm_runtime_get_sync`
* `pm_runtime_put`

### 5. Suspend / Resume Support

Implements `dev_pm_ops` for:

* System suspend
* System resume

### 6. Performance Measurement

Tracks:

* Toggle count
* Average toggle latency (nanoseconds)

Statistics are exposed via sysfs.

---

## Project Structure

```
platform_led_driver/
 ├── led_driver.c
 ├── led_driver.h
 ├── Makefile
 └── led_device.dts
```

---

## Hardware Register Map (Simulated)

| Offset | Register       | Description      |
| ------ | -------------- | ---------------- |
| 0x00   | CTRL_REG       | Enable LED       |
| 0x04   | STATUS_REG     | Status bit       |
| 0x08   | TOGGLE_REG     | Toggle LED       |
| 0x0C   | INT_STATUS_REG | Interrupt status |
| 0x10   | INT_ENABLE_REG | Interrupt enable |

---

## Build Instructions

Install required packages:

```
sudo apt install build-essential linux-headers-$(uname -r)
```

Build the module:

```
make
```

This generates:

```
led_driver.ko
```

---

## Load the Module

```
sudo insmod led_driver.ko
dmesg | tail
```

---

## Device Tree Binding Example

```
led@10000000 {
    compatible = "custom,my-led";
    reg = <0x10000000 0x1000>;
    interrupts = <42>;
    clocks = <&clk 0>;
};
```

---

## Testing

After probe succeeds, a sysfs entry is created:

```
/sys/devices/.../toggle
```

Toggle the device:

```
echo 1 > toggle
```

Read performance statistics:

```
cat toggle
```

Example Output:

```
Toggles: 15
Avg latency(ns): 1280
```

---

## Remove Module

```
sudo rmmod led_driver
```

---

## Kernel Concepts Demonstrated

* Platform driver model
* Device Tree matching
* MMIO programming
* regmap abstraction
* IRQ handling
* Clock gating
* Runtime PM
* Concurrency control (spinlock)
* Sysfs interface
* Kernel timing APIs (`ktime_get`)

---

## Industry Relevance

This driver mirrors the architecture used in:

* Sensor drivers
* Audio codec drivers
* Display controllers
* Power regulators
* SoC peripheral blocks

It demonstrates understanding of:

* Register-level hardware programming
* Performance-aware driver design
* Power management integration
* Embedded Linux kernel development

---

## Future Improvements

* Add workqueue bottom-half handler
* Integrate debugfs interface
* Add DMA simulation
* Support multiple device instances
* Add profiling with `perf` and `ftrace`

---

