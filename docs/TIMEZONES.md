# Timezone Configuration Guide

## Common Timezones

Add these values to `CONFIG_TIMEMACHINE_TIMEZONE` in `sdkconfig.defaults` or via `idf.py menuconfig`.

### Americas

| Location | Timezone String |
|----------|----------------|
| UTC | `UTC0` |
| EST (New York) | `EST5EDT,M3.2.0/2,M11.1.0` |
| CST (Chicago) | `CST6CDT,M3.2.0/2,M11.1.0` |
| MST (Denver) | `MST7MDT,M3.2.0/2,M11.1.0` |
| PST (Los Angeles) | `PST8PDT,M3.2.0,M11.1.0` |
| Argentina | `<-03>3` |
| Brazil (São Paulo) | `<-03>3` |

### Europe

| Location | Timezone String |
|----------|----------------|
| GMT (London) | `GMT0BST,M3.5.0/1,M10.5.0` |
| CET (Paris, Berlin) | `CET-1CEST,M3.5.0,M10.5.0/3` |
| EET (Athens) | `EET-2EEST,M3.5.0/3,M10.5.0/4` |

### Asia

| Location | Timezone String |
|----------|----------------|
| India (IST) | `IST-5:30` |
| China (CST) | `CST-8` |
| Japan (JST) | `JST-9` |
| Korea (KST) | `KST-9` |

### Oceania

| Location | Timezone String |
|----------|----------------|
| Australia (Sydney) | `AEST-10AEDT,M10.1.0,M4.1.0/3` |
| New Zealand | `NZST-12NZDT,M9.5.0,M4.1.0/3` |

## Format Explanation

The timezone string format is: `STD offset [DST [offset] [,start [/time], end [/time]]]`

- **STD**: Standard timezone abbreviation (e.g., PST, EST)
- **offset**: Hours from UTC (negative east of UTC, positive west)
- **DST**: Daylight saving time abbreviation (optional)
- **start/end**: DST start and end dates (optional)

### Examples

1. **Simple (no DST):**
   - `UTC0` - UTC timezone
   - `JST-9` - Japan Standard Time (UTC+9)

2. **With DST:**
   - `EST5EDT,M3.2.0/2,M11.1.0`
     - EST = Eastern Standard Time
     - 5 = 5 hours behind UTC
     - EDT = Eastern Daylight Time
     - M3.2.0/2 = 2nd Sunday in March at 2:00 AM
     - M11.1.0 = 1st Sunday in November at 2:00 AM

## Testing Your Timezone

After configuring your timezone:

```bash
# Rebuild and run
idf.py build
idf.py qemu monitor

# Or with hardware
idf.py -p /dev/ttyUSB0 flash monitor
```

The time display should show the correct local time after NTP sync.

## References

- [GNU C Library Timezone Format](https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html)
- [POSIX Timezone Strings](https://developer.ibm.com/articles/au-aix-posix/)
