This project consists of two stages, each uploaded in separate branches.

In the 'main' branch, you will find the first stage, which involves creating a fully functional simulation of the Medibox on Wokwi. This stage includes:
1. A menu with options to set time zone, set 3 alarms, and disable all alarms.
2. Fetching and displaying the current time from the NTP server over Wi-Fi on the OLED.
3. Ringing the alarm with proper indication when set alarm times are reached and stopping the alarm with a push button.
4. Monitoring temperature and humidity levels, providing warnings when limits are exceeded.

The second stage, found in the 'stage 2' branch, focuses on monitoring light intensity for medicine storage:
1. Using two Light Dependent Resistors (LDRs) to measure light intensity and displaying the highest value on a Node-RED dashboard.
2. Controlling a shaded sliding window with a servo motor to adjust light intensity based on lighting conditions, ensuring optimal storage conditions.
3. Allowing users to adjust the minimum angle and controlling factor for the shaded sliding window via sliders in the Node-RED dashboard.
4. Including a dropdown menu for selecting commonly used medicines, with predefined values for the window's minimum angle and controlling factor, or a custom option for manual adjustments.
