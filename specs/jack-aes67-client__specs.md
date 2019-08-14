# JACK AES67 Client

## Software Requirements Specification

*Created by Robin SHAMSNEJAD on 2019-Aug-13*

# Introduction

## Purpose

Proper AES67 support is not present at the moment on GNU/Linux. This project intends to fix that, by creating a software set allowing to receive and stream audio on an AES67 network in a user-freindly fashion.
Porting to Windows and macOS is kept in mind but will be distant-future work.

## Document Conventions



## Intended Audience and Reading Suggestions

\<Describe the different types of reader that the document is intended for, such
as developers, project managers, marketing staff, users, testers, and
documentation writers. Describe what the rest of this SRS contains and how it is
organized. Suggest a sequence for reading the document, beginning with the
overview sections and proceeding through the sections that are most pertinent to
each reader type.\>

## Product Scope

This software shall be a JACK client acting as a gateway between AES67 streams available on the network the computer is connected to, and other JACK clients. It is not intended to do any file operation, any resampling or anything else like that. Just handing streams to the JACK server and the other way around.

The benefits of making it a JACK client is that it allows the user to monitor audio through a regular audio interface, as well as to use any recording or playback software that supports JACK.

Other ways that have been considered :

* A virtual ALSA driver

This would have required using the ALSA API, which is an undocumented mess, despite being functional. Moreover, it would not have allowed the user to monitor using an audio interface. There could have been the workaround of using alsa_in and alsa_out for JACK, but it is really not a clean way of doing it.

* A standalone Software

This would have required writing an actual recording/playback software, which is really not interesting and too time-consuming considering that there are some perfectly good ones already. Also it would lack flexibility.

#### Short-term goals

##### Backend

- AES67 receiving :
  - Stream Bit depth : L16, L24
  - Stream Sample rate : 44.1kHz, 48kHz
  - Stream Channel count : 1, 2
  - Number of streams : 2 Mono or 1 Stereo

##### Frontend

- Types of human interfaces :
  - Unix-like CLI
- Audio section :
  - Patching of the incoming streams
  - Patching of the outgoing streams

#### Mid-term goals

##### Backend

- AES67 streaming and receiving :
  - Stream Bit depth : L16, L24
  - Stream Sample rate : 44.1kHz, 48kHz
  - Stream Channel count : 1, 2
  - Number of streams : 16 Mono or 8 Stereo
- PTPv2 (IEEE 1588-2008) slave-only synchronization :
  - Hardware timestamping
- Publishing and Discovery :
  - SAP (Dante-like)
  - mDNS+DNS-SD (RAVENNA-like)

##### Frontend

- Types of human interfaces :
  - Unix-like CLI
- Control over the backend daemon :
  - Start/Stop/Restart
  - Kill
- Audio section :
  - Discovery and patching of the incoming streams
  - Patching of outgoing streams
- Sync section :
  - Simplified PTP settings with presets

#### Long-term goals

##### Backend

- AES67 streaming and receiving :
  - Stream Bit depth : L16, L24
  - Stream Sample rate : 44.1kHz, 48kHz, 88.2kHz, 96kHz
  - Stream Channel count : 1, 2, 4, 8, 16
  - Number of streams : As much as the system can handle
- PTPv2 (IEEE 1588-2008) slave-only synchronization :
  - Software timestamping
  - Hardware timestamping
- Publishing and Discovery :
  - SAP (Dante-like)
  - mDNS+DNS-SD (RAVENNA-like)

##### Frontend

- Types of human interfaces :
  - Unix-like CLI
  - nCurses-based TUI
  - Qt-based GUI
- Full control over the backend daemon :
  - Start/Stop/Restart
  - Kill
  - Return to "factory" defaults
- Audio section :
  - Discovery and patching of the incoming streams
  - Patching of outgoing streams
  - In/Out Monitoring through peakmeters
- Sync section :
  - Simplified PTP settings with presets
  - Monitoring of the offset to the Grandmaster
- Network section :
  - IP configuration
  - Status and bandwidth monitoring

## References

- AES67-2018
- IEEE 1588-2008

# Overall Description

## Product Perspective

As stated in the introduction, open-source software-based AES67 support for end users is non-existant for GNU/Linux at the moment.

Despite having a lot of excellent audio tools, audio production on GNU/Linux is not very widespread mostly due to the lack of drivers for audio hardware. Apart from simple class-compliant USB interfaces, it is hard to find compatible hardware for extended use. This project is aiming to change that by bringing AES67 support to GNU/Linux : that way, the users will be able to use an fast-growing amount of professional audio hardware through a simple Ethernet interface.

## Product Functions

\<Summarize the major functions the product must perform or must let the user
perform. Details will be provided in Section 3, so only a high level summary
(such as a bullet list) is needed here. Organize the functions to make them
understandable to any reader of the SRS. A picture of the major groups of
related requirements and how they relate, such as a top level data flow diagram
or object class diagram, is often effective.\>

## User Classes and Characteristics

Possible users for this software :

- Amateur music producers
  - Possibly skilled in using GNU/Linux
  - Not skilled in using AES67 networks
- Professional audio technicians
  - Not skilled in using GNU/Linux
  - Possibly skilled in using AES67 networks
- AV integration engineers
  - Possibly skilled in using GNU/Linux
  - Possibly skilled in using AES67 networks


## Operating Environment

#### Software :
- GNU/Linux starting from kernel 3.0 (needed for PTP hardware timestamping)
- JACK2 server

#### Hardware :
- Any computer able to run the software requirements
- A gigabit Ethernet interface, with optional IEEE 1588-2008 support if hardware timestamping is to be used

## Design and Implementation Constraints

- Programming language : C/C++
- Libraries and external software to be used :
  - Linux kernel > 3.0
  - Audio I/O : JACK2
  - PTP Sync : LinuxPTP
  - RTP : **_TODO_** :
    - jRTPlib
    - ccRTP
    - PJSIP
  - SIP : **_TODO_**
    - PJSIP
  - CLI : getopt
  - TUI : ncurses
  - GUI : Qt
- Version control : git



## User Documentation

- Online user manual with mobile version
- PDF User manual exported from the online version, provided along with the software. A computer connected to an AES67 network is likely not to be connected to the Internet at the same time, so the user should be able to find the most offline resources as possible.
- Online wiki about JACK and AES67, as both are not very well-understood concepts.
- Man pages
- On the long term : Video tutorials

## Assumptions and Dependencies

- Software timestamping with LinuxPTP is currently a problem for this application. The system clock (CLOCK_REALTIME) is synchronized to the PTP Grandmaster, which is not always in sync with the actual wall clock. For example, a Dante or RAVENNA device boots at timestamp 0 (1970-Jan-01 00:00). When the slave GNU/Linux system reverts its system clock so far back in time, multiple problems appear on reboot. Among them are a force filesystem check, and the need to reset the hardware RTC.
  - Workarounds :
    - The user could set up an actual Grandmaster clock slaved to UTC in his network
    - The program could be a temporary Grandmaster to distribute wall clock time before reverting to slave (not ideal)
  - Possible fixes :
    - Find a way to make LinuxPTP's ptp4l use another software clock than CLOCK_REALTIME
    - Find a way to create a virtual PHC for LinuxPTP's phc2sys to use



# External Interface Requirements

## User Interfaces

- The TUI and GUI should be most user-friendly. Most of the users are not likely to be computer or network geeks, so they should be taken by the hand since AES67 is a pretty difficult subject.
- The CLI should be as scriptable as possible.

## Hardware Interfaces

- One Ethernet interface, with optional PHC if hardware timestamping is to be used

## Software Interfaces

- Linux Kernel > 3.0 for timing control and Ethernet I/O
- LinuxPTP daemon for PTP clock sync and monitoring
- JACK daemon for audio I/O

## Communications Interfaces

- Network protocols to be used : AES67-defined, including but not limited to :
 - UDP over IPv4
 - PTPv2 for time sync
 - RTP/RTCP
 - SIP for unicast connection management
 - SAP for Dante multicast discovery and connection management
 - mDNS/DNS-SD for RAVENNA multicast discovery and connection management

# System Features

\<This template illustrates organizing the functional requirements for the
product by system features, the major services provided by the product. You may
prefer to organize this section by use case, mode of operation, user class,
object class, functional hierarchy, or combinations of these, whatever makes the
most logical sense for your product.\>

## System Feature 1

\<Don’t really say “System Feature 1.” State the feature name in just a few
words.\>

### 4.1.1 Description and Priority

\<Provide a short description of the feature and indicate whether it is of High,
Medium, or Low priority. You could also include specific priority component
ratings, such as benefit, penalty, cost, and risk (each rated on a relative
scale from a low of 1 to a high of 9).\>

### 4.1.2 Stimulus/Response Sequences

\<List the sequences of user actions and system responses that stimulate the
behavior defined for this feature. These will correspond to the dialog elements
associated with use cases.\>

### 4.1.3 Functional Requirements

\<Itemize the detailed functional requirements associated with this feature.
These are the software capabilities that must be present in order for the user
to carry out the services provided by the feature, or to execute the use case.
Include how the product should respond to anticipated error conditions or
invalid inputs. Requirements should be concise, complete, unambiguous,
verifiable, and necessary. Use “TBD” as a placeholder to indicate when necessary
information is not yet available.\>

\<Each requirement should be uniquely identified with a sequence number or a
meaningful tag of some kind.\>

REQ-1:

REQ-2:

## System Feature 2 (and so on)

# Other Nonfunctional Requirements

## Performance Requirements

\<If there are performance requirements for the product under various
circumstances, state them here and explain their rationale, to help the
developers understand the intent and make suitable design choices. Specify the
timing relationships for real time systems. Make such requirements as specific
as possible. You may need to state performance requirements for individual
functional requirements or features.\>

## Safety Requirements

\<Specify those requirements that are concerned with possible loss, damage, or
harm that could result from the use of the product. Define any safeguards or
actions that must be taken, as well as actions that must be prevented. Refer to
any external policies or regulations that state safety issues that affect the
product’s design or use. Define any safety certifications that must be
satisfied.\>

## Security Requirements

\<Specify any requirements regarding security or privacy issues surrounding use
of the product or protection of the data used or created by the product. Define
any user identity authentication requirements. Refer to any external policies or
regulations containing security issues that affect the product. Define any
security or privacy certifications that must be satisfied.\>

## Software Quality Attributes

\<Specify any additional quality characteristics for the product that will be
important to either the customers or the developers. Some to consider are:
adaptability, availability, correctness, flexibility, interoperability,
maintainability, portability, reliability, reusability, robustness, testability,
and usability. Write these to be specific, quantitative, and verifiable when
possible. At the least, clarify the relative preferences for various attributes,
such as ease of use over ease of learning.\>

## Business Rules

\<List any operating principles about the product, such as which individuals or
roles can perform which functions under specific circumstances. These are not
functional requirements in themselves, but they may imply certain functional
requirements to enforce the rules.\>

# Other Requirements

\<Define any other requirements not covered elsewhere in the SRS. This might
include database requirements, internationalization requirements, legal
requirements, reuse objectives for the project, and so on. Add any new sections
that are pertinent to the project.\>

# Appendix A: Glossary

- RTP : Real-Time protocols
- PTP : Precision Time protocols
- AES67 : Audio network standard from Audio Engineering Society
- CLI = Command Line Interface
- TUI = Text-based User Interface (typically ncurses)
- GUI = Graphical User Interface

# Appendix B: Analysis Models

\<Optionally, include any pertinent analysis models, such as data flow diagrams,
class diagrams, state-transition diagrams, or entity-relationship diagrams.\>

# Appendix C: To Be Determined List

\<Collect a numbered list of the TBD (to be determined) references that remain
in the SRS so they can be tracked to closure.\>
