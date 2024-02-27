# Nutrient Dosing System

## System diagram
![alt text](https://github.com/msahmed1/nutrient-dosing-system/blob/main/system%20diagram.png)

## Hardware
| Part                          | Quantity           | Description                                          |
|-------------------------------|--------------------|------------------------------------------------------|
| Reservoir/mixing tank         | 1                  | Nutrient mixed and stored                            |
| Sump tank                     | 1                  | Stores spent nutrients for recirculation             |
| Relay                         | 9                  |                                                      |
| Agitator pump                 | 1                  | Mix nutrient solutions                               |
| Drain pump                    | 1                  | Encase nutrient concentration is too high            |
| Fill pump                     | 2                  | Freshwater or recirculated nutrients top up          |
| Flow sensors                  | 4                  | To measure the total flow of solution                |
| 1” bulkhead fittings          | 2                  | Passively drain if nutrient level gets too high      |
| pH sensor                     | 1                  | Need 2 for redundancy                                |
| EC sensor                     | 1                  | Need 2 for redundancy                                |
| BNC panel mount               | 2                  | For making nutrient controller water tight           |
| Float switch                  | 3                  | For low and high-water level sensing                 |
| Waterproof temperature sensor | 2                  | For pH correction                                    |
| Peristaltic pumps             | 5                  |                                                      |
| Diodes                        | 5                  | To remove EMF when switching off pumps               |
| Momentary buttons             | 3                  | To control UI                                        |
| Resistors 10K                 | 4                  | For buttons                                          |
| 20X4 I2C LCD                  | 1                  | To display information                               |
| 12V Power Supply              | 1                  | To power 12V components                              |
| 5V power supply               | 1                  | To power Arduino and sensors                         |
| Electrical Wire               |                    |                                                      |
| 90 dual rail header pins      | 1                  | To allow access to unused I/O pins                   |
| 90 single rail header pins    | 1                  | To allow access to unused I/O pins                   |
| Power Switch                  | 1                  | For emergency shutdown                               |
| 120mm computer fan            | 5                  | Component for magnetic stirrer                       |
| 15mm neodymium magnets        | 10                 | Component for magnetic stirrer                       |
| Magnetic stirrer bar          | 5                  | Component for magnetic stirrer                       |
| 1 liter bottles               | 5                  | Storing nutrient and buffer solutions                |
| M4 nuts and bolts             | 29                 |                                                      |
| M4 rubber washers             | 9                  | For sealing electronic box mounting holes            |
| Rubber gasket cord (5m)       | 1                  | For sealing electronic box lid                       |
| Liquid level sensor           | 1                  | For measuring actual water level                     |

## Set up
1. Calibrate peristaltic pumps
2. Fill the tank to the first float switch
3. Fill beakers with nutrients
4. Calibrate pH and EC
