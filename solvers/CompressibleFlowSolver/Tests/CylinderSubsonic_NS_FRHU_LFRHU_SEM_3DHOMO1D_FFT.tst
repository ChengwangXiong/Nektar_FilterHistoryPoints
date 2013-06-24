<?xml version="1.0" encoding="utf-8"?>
<test>
    <description>NS, Subsonic Cylinder, mixed bcs, FRHU advection and LFRHU diffusion, SEM</description>
    <executable>CompressibleFlowSolver</executable>
    <parameters>CylinderSubsonic_NS_FRHU_LFRHU_SEM_3DHOMO1D_FFT.xml</parameters>
    <files>
        <file description="Session File">CylinderSubsonic_NS_FRHU_LFRHU_SEM_3DHOMO1D_FFT.xml</file>
    </files>
    <metrics>
        <metric type="L2" id="1">
            <value variable="rho" tolerance="1e-12">3.89459</value>
            <value variable="rhou" tolerance="1e-12">1425.75</value>
            <value variable="rhov" tolerance="1e-12">17.5075</value>
            <value variable="rhow" tolerance="1e-12">17.3092</value>
            <value variable="E" tolerance="1e-12">4.43372e+06</value>
        </metric>
        <metric type="Linf" id="2">
            <value variable="rho" tolerance="1e-12">0.352389</value>
            <value variable="rhou" tolerance="1e-12">84.4924</value>
            <value variable="rhov" tolerance="1e-8">25.8156</value>
            <value variable="rhow" tolerance="1e-12">1.00007</value>
            <value variable="E" tolerance="1e-12">293745</value>
        </metric>
    </metrics>
</test>


