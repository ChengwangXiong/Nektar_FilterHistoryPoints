<?xml version="1.0" encoding="utf-8"?>
<test>
    <description>Helmholtz 3D CG for Tet</description>
    <executable>Helmholtz3D</executable>
    <parameters>Helmholtz3D_Tet.xml</parameters>
    <files>
        <file description="Session File">Helmholtz3D_Tet.xml</file>
    </files>

    <metrics>
        <metric type="L2" id="1">
            <value tolerance="1e-12">5.48966e-05</value>
        </metric>
        <metric type="Linf" id="2">
            <value tolerance="1e-12">0.000344037</value>
        </metric>
    </metrics>
</test>


