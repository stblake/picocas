import { useMemo } from 'react';
import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  ResponsiveContainer,
} from 'recharts';
import type { PlotData } from '../hooks/useNotebook';

interface PlotOutputProps {
  plotData: PlotData;
}

export function PlotOutput({ plotData }: PlotOutputProps) {
  const data = useMemo(
    () => plotData.points.map(([x, y]) => ({ x, y })),
    [plotData.points]
  );

  return (
    <div className="cell-output-plot">
      <ResponsiveContainer width="100%" height={220}>
        <LineChart data={data} margin={{ top: 8, right: 16, bottom: 8, left: 0 }}>
          <CartesianGrid strokeDasharray="3 3" stroke="#e0e0e0" />
          <XAxis
            dataKey="x"
            type="number"
            domain={[plotData.xMin, plotData.xMax]}
            tickCount={7}
            tickFormatter={(v: number) => v.toFixed(2)}
            tick={{ fontSize: 11 }}
          />
          <YAxis
            type="number"
            domain={['auto', 'auto']}
            tickFormatter={(v: number) => v.toFixed(2)}
            tick={{ fontSize: 11 }}
            width={48}
          />
          <Tooltip
            formatter={(v) => (typeof v === 'number' ? v.toFixed(4) : String(v))}
            labelFormatter={(l) => `${plotData.varName} = ${Number(l).toFixed(4)}`}
          />
          <Line
            type="monotone"
            dataKey="y"
            stroke="#2563eb"
            strokeWidth={2}
            dot={false}
            isAnimationActive={false}
          />
        </LineChart>
      </ResponsiveContainer>
    </div>
  );
}
