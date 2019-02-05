package main

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/require"
	"github.com/tarm/serial"
)

func TestSerial(t *testing.T) {
	buf := make([]byte, 8)

	c := &serial.Config{Name: "/dev/rfcomm0", Baud: 9600}
	s, err := serial.OpenPort(c)
	require.NoError(t, err)
	defer s.Close()
	n, err := s.Write([]byte{0xa6})
	require.NoError(t, err)
	n, err = s.Read(buf)
	require.NoError(t, err)
	require.Equal(t, 1, n)
	if buf[0] != 0x20 && buf[0] != 0x00 {
		t.Fail()
	}
	fmt.Printf("read %d bytes. data: %v", n, buf[:n])
}
