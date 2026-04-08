import { Notebook } from './components/Notebook';
import { useNotebook } from './hooks/useNotebook';
import './App.css';

export default function App() {
  const notebook = useNotebook();
  return <Notebook notebook={notebook} />;
}
