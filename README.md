# The Refinement Terminal

### A Diegetic UI Prototype in Unreal Engine 5
**Inspired by the retro-corporate aesthetic of *Severance*.**

![Project_Ref_Promo-ezgif com-video-to-gif-converter](https://github.com/user-attachments/assets/e084c748-dbde-4103-a780-322bcb6c1033)

## 📖 Project Overview
This project is a technical showcase focused on **User Interface (UI) architecture** and **Widget Logic** within Unreal Engine 5. 

The goal was to replicate the "analog-digital" feel of 1980s corporate terminals while maintaining a modern, performant backend. It demonstrates a "code-first" approach to UI, prioritizing efficient data binding and modular C++ classes over heavy Blueprint scripting.

## 🛠 Technical Implementation
This prototype utilizes a hybrid **C++ & Blueprint** workflow to maximize performance and flexibility:

* **C++ Base Classes:** All core widget logic derives from custom C++ classes (inheriting from `UUserWidget`), ensuring separation of concerns between game logic and visual presentation.
* **Optimized Data Binding:** Utilized native C++ delegates and event dispatchers to update UI elements, avoiding the performance overhead of property bindings or tick-based updates.
* **Responsive Layouts:** Leveraged UE5's Anchors and Scale Boxes to ensure the interface scales correctly across different resolutions and aspect ratios.
* **Material Shaders:** Implemented custom material logic to simulate CRT monitor effects, including scanlines, curvature, and chromatic aberration.

## 💻 Code Highlights
* **`URefinementWidget` (C++):** The primary base class handling the state machine for the terminal's menus.
* **Delegate Architecture:** The system uses a centralized `EventManager` to broadcast state changes (e.g., completing a task, receiving a notification) to the UI without hard references.

## 🎨 Aesthetic & Design
* **Visual Style:** Minimalist, sterile corporate typography paired with looping background elements.
* **Animation:** Widget animations are driven by state changes to create smooth, "eased" transitions typical of older operating systems.

## 🔧 Installation & Setup
1. Go to ItchIO: https://schwiftygg.itch.io
2. Download and Unzip.
3. Press the Unreal .exe and play!

---
*Note: This is a non-commercial educational prototype inspired by the TV show Severance. All concepts are used for portfolio demonstration purposes.*

## Note

This repository contains C++ source code only. Full project includes Blueprint assets, materials, and animations not included in this repo due to size constraints.
