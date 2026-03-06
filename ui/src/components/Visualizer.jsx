import React, { useRef } from 'react';
import { Canvas, useFrame } from '@react-three/fiber';
import { OrbitControls, Icosahedron, Box, Edges, Float, MeshDistortMaterial } from '@react-three/drei';

function MeshGenerator({ drive, randomize, tonic }) {
    const meshRef = useRef();

    useFrame((state) => {
        const t = state.clock.getElapsedTime();
        if (meshRef.current) {
            meshRef.current.rotation.y = t * (0.15 + tonic * 0.001);
            meshRef.current.rotation.x = Math.sin(t * 0.2) * 0.1;
        }
    });

    const scale = 0.8 + (drive * 0.1);

    const edgeProps = {
        scale: 1.0,
        threshold: 15,
        color: "white",
        renderOrder: 1000,
        transparent: true,
        opacity: 0.35
    };

    return (
        <Float speed={1 + randomize * 5} rotationIntensity={1} floatIntensity={1}>
            <mesh ref={meshRef} scale={[scale, scale, scale]}>
                <icosahedronGeometry args={[1.5, Math.max(1, Math.floor(drive))]} />
                <meshBasicMaterial transparent opacity={0} />
                <Edges {...edgeProps} />

                {/* Inner glowing core that responds to drive */}
                <mesh scale={[0.8, 0.8, 0.8]}>
                    <icosahedronGeometry args={[1.5, Math.max(0, Math.floor(drive) - 1)]} />
                    <MeshDistortMaterial
                        color="#ffffff"
                        wireframe={true}
                        distort={0.2 + (randomize * 0.5)}
                        speed={1 + (drive * 0.5)}
                        roughness={0.2}
                        metalness={0.8}
                        emissive="#ff0044"
                        emissiveIntensity={0.1 + (drive * 0.05)}
                        transparent
                        opacity={0.2}
                    />
                </mesh>
            </mesh>
        </Float>
    );
}

export default function Visualizer({ drive = 1.0, randomize = 0.2, tonic = 60 }) {
    return (
        <div style={{ width: '100%', height: '100%', position: 'relative', background: '#050505' }}>
            <Canvas camera={{ position: [0, 0, 5], fov: 45 }} dpr={[1, 2]}>
                <ambientLight intensity={0.5} />
                <MeshGenerator drive={drive} randomize={randomize} tonic={tonic} />
                <OrbitControls enableZoom={false} enablePan={false} autoRotate={false} />
            </Canvas>

            {/* Overlay Text */}
            <div style={{
                position: 'absolute',
                bottom: '30px',
                left: '30px',
                pointerEvents: 'none',
                textAlign: 'left'
            }}>
                <h2 style={{ fontSize: '20px', fontWeight: 600, color: '#fff', margin: 0, letterSpacing: '-0.5px' }}>messina</h2>
                <p style={{ fontSize: '12px', color: '#666', marginTop: '4px', letterSpacing: '0.5px', textTransform: 'lowercase' }}>polyphonic tonic harmonizer</p>
            </div>
        </div>
    );
}
