// ================================================
// ADVANCED_ENUMERABLE_TASKS.cs
// Zbiór złożonych wariantów zadań analogicznych
// do Batch / SlidingWindow / InOrder / LINQ
// Gotowy do kompilacji i modyfikacji.
// ================================================

using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

public static class AdvancedEnumerableExtensions
{
    // ====================================================
    // 1. BatchWithRemainder — wersja Batch z obsługą pozostałości
    // ====================================================
    // Zadanie analogiczne do Batch, ale ostatnia partia musi mieć co najmniej połowę rozmiaru,
    // w przeciwnym razie zostaje połączona z poprzednią partią.
    public static IEnumerable<IEnumerable<T>> BatchWithRemainder<T>(this IEnumerable<T> collection, int size)
    {
        if (size < 1)
            throw new ArgumentOutOfRangeException(nameof(size), "Batch size must be at least 1.");

        using var enumerator = collection.GetEnumerator();
        List<T>? prevBatch = null;

        while (enumerator.MoveNext())
        {
            var batch = new List<T>(size) { enumerator.Current };

            for (int i = 1; i < size && enumerator.MoveNext(); i++)
                batch.Add(enumerator.Current);

            if (prevBatch != null && batch.Count < size / 2)
            {
                prevBatch.AddRange(batch);
                yield return prevBatch;
                yield break;
            }
            else if (prevBatch != null)
                yield return prevBatch;

            prevBatch = batch;
        }

        if (prevBatch != null)
            yield return prevBatch;
    }

    // ====================================================
    // 2. SlidingWindowWithStep — przesuwanie o krok > 1
    // ====================================================
    // Analogiczne do SlidingWindow, ale pozwala na ustalony krok przesunięcia.
    public static IEnumerable<T[]> SlidingWindowWithStep<T>(this IEnumerable<T> source, int size, int step)
    {
        if (size < 1 || step < 1)
            throw new ArgumentException("Window size and step must be at least 1.");

        var buffer = new Queue<T>();
        using var enumerator = source.GetEnumerator();
        while (enumerator.MoveNext())
        {
            buffer.Enqueue(enumerator.Current);
            if (buffer.Count > size)
                buffer.Dequeue();
            if (buffer.Count == size)
            {
                yield return buffer.ToArray();
                for (int i = 0; i < step - 1 && enumerator.MoveNext(); i++)
                    buffer.Enqueue(enumerator.Current);
            }
        }
    }

    // ====================================================
    // 3. Pairwise — analogiczne do SlidingWindow(2)
    // ====================================================
    public static IEnumerable<(T First, T Second)> Pairwise<T>(this IEnumerable<T> source)
    {
        using var enumerator = source.GetEnumerator();
        if (!enumerator.MoveNext()) yield break;
        T prev = enumerator.Current;
        while (enumerator.MoveNext())
        {
            yield return (prev, enumerator.Current);
            prev = enumerator.Current;
        }
    }

    // ====================================================
    // 4. WindowStatistics — przykład wykorzystania SlidingWindow
    // ====================================================
    public static IEnumerable<(T[] Window, double Average)> WindowStatistics(this IEnumerable<int> source, int size)
    {
        foreach (var w in source.SlidingWindow(size))
        {
            double avg = w.Average();
            yield return (w, avg);
        }
    }

    // ====================================================
    // 5. ChunkByPredicate — analogiczne do Batch, ale według warunku
    // ====================================================
    // Dzieli kolekcję na grupy rozdzielone elementami spełniającymi predykat separator.
    public static IEnumerable<List<T>> ChunkByPredicate<T>(this IEnumerable<T> source, Func<T, bool> separator)
    {
        var chunk = new List<T>();
        foreach (var item in source)
        {
            if (separator(item))
            {
                if (chunk.Count > 0)
                {
                    yield return chunk;
                    chunk = new List<T>();
                }
            }
            else
                chunk.Add(item);
        }
        if (chunk.Count > 0)
            yield return chunk;
    }

    // ====================================================
    // 6. EnumerateTreeInPostOrder — wariant traversal iteracyjny
    // ====================================================
    public static IEnumerable<T> EnumerateTreeInPostOrder<T>(this BinaryTreeNode<T>? root)
    {
        if (root == null) yield break;
        var stack = new Stack<(BinaryTreeNode<T> Node, bool Visited)>();
        stack.Push((root, false));

        while (stack.Count > 0)
        {
            var (node, visited) = stack.Pop();
            if (node == null) continue;

            if (visited)
                yield return node.Value;
            else
            {
                stack.Push((node, true));
                stack.Push((node.Right, false));
                stack.Push((node.Left, false));
            }
        }
    }

    // ====================================================
    // 7. GroupSliding — grupowanie z nakładaniem (łączenie Batch i Sliding)
    // ====================================================
    // Przykład: dla [1..6], size=3, overlap=1 => [1,2,3], [3,4,5], [5,6]
    public static IEnumerable<T[]> GroupSliding<T>(this IEnumerable<T> source, int size, int overlap)
    {
        if (size < 1 || overlap < 0 || overlap >= size)
            throw new ArgumentException("Invalid window or overlap size.");

        var buffer = new Queue<T>();
        using var enumerator = source.GetEnumerator();

        while (enumerator.MoveNext())
        {
            buffer.Enqueue(enumerator.Current);
            if (buffer.Count == size)
            {
                yield return buffer.ToArray();
                for (int i = 0; i < size - overlap && buffer.Count > 0; i++)
                    buffer.Dequeue();
            }
        }

        if (buffer.Count > 0)
            yield return buffer.ToArray();
    }

    // ====================================================
    // 8. WindowZip — łączenie dwóch przesuniętych sekwencji
    // ====================================================
    // Tworzy sekwencję par (a[i], a[i+1]) — ogólny zip z offsetem.
    public static IEnumerable<(T Current, T Next)> WindowZip<T>(this IEnumerable<T> source)
    {
        using var e1 = source.GetEnumerator();
        if (!e1.MoveNext()) yield break;
        T prev = e1.Current;
        while (e1.MoveNext())
        {
            yield return (prev, e1.Current);
            prev = e1.Current;
        }
    }

    // ====================================================
    // 9. RollingSum — analogiczne do SlidingWindow z agregacją
    // ====================================================
    public static IEnumerable<int> RollingSum(this IEnumerable<int> source, int windowSize)
    {
        var queue = new Queue<int>();
        int sum = 0;
        foreach (var item in source)
        {
            queue.Enqueue(item);
            sum += item;
            if (queue.Count > windowSize)
                sum -= queue.Dequeue();
            if (queue.Count == windowSize)
                yield return sum;
        }
    }

    // ====================================================
    // 10. SlidingWindow() — bazowa wersja do wykorzystania powyżej
    // ====================================================
    public static IEnumerable<T[]> SlidingWindow<T>(this IEnumerable<T> source, int size)
    {
        if (size < 1)
            throw new ArgumentException("Window size must be at least 1.");

        var window = new Queue<T>();
        using var enumerator = source.GetEnumerator();
        while (enumerator.MoveNext())
        {
            window.Enqueue(enumerator.Current);
            if (window.Count > size)
                window.Dequeue();
            if (window.Count == size)
                yield return window.ToArray();
        }
    }
}

// ====================================================
// Struktura drzewa dla EnumerateTreeInPostOrder
// ====================================================
public class BinaryTreeNode<T>
{
    public T Value { get; set; }
    public BinaryTreeNode<T>? Left { get; set; }
    public BinaryTreeNode<T>? Right { get; set; }

    public BinaryTreeNode(T value, BinaryTreeNode<T>? left = null, BinaryTreeNode<T>? right = null)
    {
        Value = value;
        Left = left;
        Right = right;
    }
}

// ====================================================
// Przykład użycia
// ====================================================
public class Program
{
    public static void Main()
    {
        var data = Enumerable.Range(1, 7);

        Console.WriteLine("BatchWithRemainder:");
        foreach (var b in data.BatchWithRemainder(3))
            Console.WriteLine($"[{string.Join(", ", b)}]");

        Console.WriteLine("\nSlidingWindowWithStep:");
        foreach (var w in data.SlidingWindowWithStep(3, 2))
            Console.WriteLine($"[{string.Join(", ", w)}]");

        Console.WriteLine("\nPairwise:");
        foreach (var p in data.Pairwise())
            Console.WriteLine($"({p.First}, {p.Second})");

        Console.WriteLine("\nRollingSum (size=3):");
        foreach (var s in data.RollingSum(3))
            Console.WriteLine(s);
    }
}
