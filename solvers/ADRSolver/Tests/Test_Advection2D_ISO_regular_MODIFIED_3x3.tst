<?xml version="1.0" encoding="utf-8"?>
<test>
    <description>2D unsteady FRHU advection MODIFIED, P=10, homogeneous Dirichlet bcs, regular elements</description>
    <executable>ADRSolver</executable>
    <parameters>Test_Advection2D_ISO_regular_MODIFIED_3x3.xml</parameters>
    <files>
        <file description="Session File">Test_Advection2D_ISO_regular_MODIFIED_3x3.xml</file>
    </files>
    <metrics>
        <metric type="L2" id="1">
            <value variable="u" tolerance="1e-12"> 0.00332013 </value>
        </metric>
        <metric type="Linf" id="2">
            <value variable="u" tolerance="1e-12"> 0.0418033 </value>
        </metric>
    </metrics>
</test>